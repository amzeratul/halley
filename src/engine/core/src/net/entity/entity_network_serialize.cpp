#include "halley/net/entity/entity_network_serialize.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/entity/world.h"
#include "halley/entity/components/transform_2d_component.h"

using namespace Halley;

static constexpr uint16_t SERIALIZE_CHECK_PARITY = 0xbaad;

static void injectMagic(Serializer& serializer, uint16_t n)
{
    serializer << gsl::span(reinterpret_cast<std::byte *>(&n), sizeof(n));
}

static void checkMagic(Deserializer& deserializer, uint16_t n)
{
    uint16_t m;
    deserializer >> gsl::span(reinterpret_cast<std::byte *>(&m), sizeof(m));

    if (m != n) {
        throw Exception("Network stream error", HalleyExceptions::Network);
    }
}

thread_local Bytes EntityNetworkSerialize::scratchpad;

void EntityNetworkChanges::beginPage(Serializer& serializer, Type type)
{
    curPage.hash = 0;
    curPage.from = static_cast<uint32_t>(serializer.getPosition());
    curPage.to = curPage.from;

    curPage.type = type;

    curPage.modified = true;
}

void EntityNetworkChanges::endPage(Serializer& serializer, Bytes& buffer, Type type)
{
    curPage.to = static_cast<uint32_t>(serializer.getPosition());

    Ensures(curPage.type == type);
    Ensures(curPage.to >= curPage.from);

    size_t size = curPage.to - curPage.from;

    if (size > 0 && pp < pages.size()) {
        // hash content of the current page
        curPage.hash = Hash::hash(buffer.const_byte_span().subspan(curPage.from, size));

        // append to pages
        pages[pp] = curPage;
        pp++;

        // update "global" hash
        contentHasher.feed(curPage.hash);
    }
}

void EntityNetworkChanges::pushEntity(Serializer& serializer, const EntityRef& entity, std::optional<EntityRef> parent, Bytes& buffer)
{
    // Entity "header"
    beginPage(serializer, Type::Entity);
    curPage.uuid = entity.getInstanceUUID();

    serializer << entity.getInstanceUUID();

    uint8_t flags = 0;
    if (!entity.isSelectable()) flags |= static_cast<uint8_t>(EntityData::Flag::NotSelectable);
    if (!entity.isSerializable()) flags |= static_cast<uint8_t>(EntityData::Flag::NotSerializable);
    if (!entity.isEnabled()) flags |= static_cast<uint8_t>(EntityData::Flag::Disabled);

    serializer << flags;

    endPage(serializer, buffer, Type::Entity);

    // Identity
    beginPage(serializer, Type::EntityIdentity);
    curPage.uuid = entity.getInstanceUUID();

    UUID parentInstanceUUID = {};
    if (parent.has_value()) {
        parentInstanceUUID = parent->getInstanceUUID();
    }

    serializer << parentInstanceUUID;
    serializer << entity.getPrefabUUID();

    serializer << entity.getName();

    //serializer << entity.getPrefabAssetId();

    endPage(serializer, buffer, Type::EntityIdentity);
}

void EntityNetworkChanges::beginComponent(Serializer& serializer, uint16_t componentId)
{
    beginPage(serializer, Type::Component);
    curPage.componentId = componentId;
}

void EntityNetworkChanges::endComponent(Serializer& serializer, Bytes& buffer)
{
    endPage(serializer, buffer, Type::Component);
}

void EntityNetworkChanges::digest()
{
    contentHash = contentHasher.digest();
}

bool EntityNetworkChanges::isFull() const
{
    return pp >= pages.size();
}

void EntityNetworkChanges::serialize(Serializer& s) const
{
    s << (uint16_t) pp;

    for (int p = 0; p < pp; p++) {
        auto& page = pages[p];
        s << page.uuid;
        s << page.hash;
        s << page.from;
        s << page.to;
        s << page.type;
    }
}

void EntityNetworkChanges::deserialize(Deserializer& s)
{
    uint16_t count;
    s >> count;
    pp = count;

    contentHasher.reset();

    for (int p = 0; p < pp; p++) {
        auto& page = pages[p];
        s >> page.uuid;
        s >> page.hash;
        s >> page.from;
        s >> page.to;
        s >> page.type;

        contentHasher.feed(page.hash);
    }

    contentHash = contentHasher.digest();
}

size_t EntityNetworkChanges::getRequiredSerializeSize() const
{
    return pp * sizeof(Page); // technically less without padding
}

bool EntityNetworkChanges::operator==(const EntityNetworkChanges& other) const
{
    bool eq = pp == other.pp;

    if (eq && pp > 0) {
        // NOTE: this should be exhaustive. If any sub-page changes,
        // the content hash will change, too.
        Expects(contentHash != 0);
        Expects(other.contentHash != 0);
        eq = contentHash == other.contentHash;
    }

    return eq;
}

// Parses the journal, along with the entity data, and writes everything to the
// output buffer. Tries to skip unmodified data along the way.
void EntityNetworkChanges::writeJournal(Serializer& serializer, const Bytes& buffer, bool log) const
{
    Expects(pp > 0);

    int p = 0;
    while (p < pp) {
        auto& page = pages[p];

        bool skip = false;
        size_t size = page.to - page.from;

        if (page.type == Type::Entity) {
            // The check for page 0 is important here - we don't want to skip the root entity!
            if (p > 0 && !page.modified) {
                skip = true;
                p++;

                // Seek forward to the next entity.

                while (p < pp) {
                    auto& next = pages[p];
                    if (next.type == Type::Entity) {
                        break;
                    }
                    Ensures(!next.modified);
                    p++;
                }
            }
        } else if (page.type == Type::EntityIdentity) {
            if (!page.modified) {
                skip = true;
                p++;
            }
        } else if (page.type == Type::Component) {
            if (!page.modified || size == 0) {
                skip = true;
                p++;
            }
        }

        if (!skip) {
            Expects(page.type == Type::Entity ||
                page.type == Type::EntityIdentity ||
                page.type == Type::Component);

            injectMagic(serializer, SERIALIZE_CHECK_PARITY);

            serializer << static_cast<uint8_t>(page.type);
            serializer << static_cast<uint32_t>(size);

            if (page.type == Type::Component) {
                // "Inject" component ID
                serializer << page.componentId;
            }

            Expects(size > 0);
            auto span = buffer.const_byte_span().subspan(page.from, size);
            serializer << span;

            if (log) {
                if (page.type == Type::Entity) {
                    if (p == 0) {
                        Logger::logInfo("  - root, " + toString(size) + " bytes");
                    } else {
                        Logger::logInfo("  - entity, " + toString(size) + " bytes");
                    }
                } else if (page.type == Type::EntityIdentity) {
                    Logger::logInfo("  - identity, " + toString(size) + " bytes");
                } else if (page.type == Type::Component) {
                    Logger::logInfo("  - component " + toString(page.componentId) + ", " + toString(size) + " bytes");
                }
            }

            p++;
        }
    }
}

void EntityNetworkChanges::enumerateEntityPages(const HashSet<UUID>& filter,
        const std::function<void (const Page& page, int pageIdx)>& onEntity) const
{
    enumerateEntities(
            [&filter, onEntity](const Page &page, int pageIdx) {
                if (pageIdx == 0 || filter.contains(page.uuid)) {
                    onEntity(page, pageIdx);
                }
            }
    );
}

void EntityNetworkChanges::enumerateEntities(const std::function<void (const Page& page, int pageIdx)>& onEntity) const
{
    for (int p = 0; p < pp; p++) {
        auto& page = pages[p];
        if (page.type == Type::Entity) {
            onEntity(page, p);
        }
    }
}

EntityNetworkChanges::Page* EntityNetworkChanges::getEntityIdentity(int pageIdx)
{
    Ensures(pageIdx + 1 < pp);
    Ensures(pages[pageIdx].type == Type::Entity);
    Ensures(pages[pageIdx + 1].type == Type::EntityIdentity);

    return &pages[pageIdx + 1];
}

EntityNetworkChanges::Page* EntityNetworkChanges::findNextComponent(int& pageIdx)
{
    while (pageIdx < pp) {
        auto& page = pages[pageIdx];

        if (page.type != Type::Component) {
            break;
        }

        pageIdx++;

        if (page.type == Type::Component) {
            return &page;
        }
    }

    return nullptr;
}

const EntityNetworkChanges::Page& EntityNetworkChanges::findEntityByUUID(const UUID& uuid, int& pageIdx) const
{
    for (int p = 0; p < pp; p++) {
        auto &page = pages[p];

        if (page.type == Type::Entity) {
            if (page.uuid == uuid) {
                pageIdx = p;
                return page;
            }
        }
    }

    throw Exception("No journal page found for entity", HalleyExceptions::Network);
}

EntityNetworkSerialize::EntityNetworkSerialize(const EntityNetworkSession* session)
    : session(session)
    , hasComponentsAddedOrRemoved(false)
{
    // Thread-local buffer to serialize/encode changes into.
    // The maximum size depends on some limits: MTU, maximum UDP packet split size in AckUnreliableConnection,
    // and maximum network message split size in EntityNetworkSession.
    scratchpad.reserve(16 * 32 * 1024);
    scratchpad.resize_no_init(scratchpad.capacity());
}

bool EntityNetworkSerialize::serializeEntityUpdate(const EntityRef& entity, const SerializerOptions& options)
{
    SerializerOptions opt(SerializerOptions::maxVersion);
    opt.dictionary = options.dictionary;
    opt.world = &entity.getWorld();

    Serializer serializer(scratchpad.byte_span(), opt);
    const SerializationContext context(entity);

    doSerializeEntityUpdate(context, serializer, entity, {});

    journal.digest();

    return !journal.isFull();
}

void EntityNetworkSerialize::doSerializeEntityUpdate(
    const SerializationContext& context, Serializer& serializer,
    const EntityRef& entity, const std::optional<EntityRef>& parent)
{
    if (!entity.isSerializable()) {
        Logger::logDev("Send network update for non-serializable entity " + entity.getPrefabAssetId(), true);
    }

    context.setCurrentEntity(entity);

    EntitySerializationContext serializationContext = {};
    serializationContext.resources = &session->getResources();
    serializationContext.entityContext = &context;
    serializationContext.interpolators = nullptr;
    serializationContext.entitySerializationTypeMask = makeMask(EntitySerialization::Type::Network);

    ByteSerializationContext byteSerializationContext = {};
    byteSerializationContext.resources = &session->getResources();
    byteSerializationContext.interpolators = session->getByteDataInterpolatorSet();
    byteSerializationContext.entityId = context.getCurrentEntityId();
    byteSerializationContext.entityInterpolators = context.getByteDataInterpolators();
    byteSerializationContext.entitySerializationContext = &serializationContext;

    // Entity
    journal.pushEntity(serializer, entity, parent, scratchpad);

    // Components
    auto& reflection = serializer.getOptions().world->getReflection();

    for (auto [componentId, component] : entity) {
        const auto& reflector = reflection.getComponentReflector(componentId);

        journal.beginComponent(serializer, (uint16_t) componentId);
        reflector.serializeNetwork(byteSerializationContext, serializer, *component);
        journal.endComponent(serializer, scratchpad);
    }

    // Children
    for (const auto& child : entity.getChildren()) {
        if (!child.isSerializable()) {
            continue;
        }
        doSerializeEntityUpdate(context, serializer, child, entity);
    }
}

EntityNetworkSerialize::InboundResult EntityNetworkSerialize::deserializeEntityUpdate(EntityRef& entity, const std::shared_ptr<const Prefab>& prefab, const Bytes& bytes, const SerializerOptions& options)
{
    SerializerOptions opt(SerializerOptions::maxVersion);
    opt.dictionary = options.dictionary;
    opt.world = &entity.getWorld();

    Deserializer deserializer(bytes, opt);
    const SerializationContext context(entity, prefab);

    EntityNetworkChanges::Type type;
    uint32_t size;
    fetchNextPage(deserializer, type, size);

    if (type != EntityNetworkChanges::Type::Entity) {
        throw Exception("Unexpected entity network change type", HalleyExceptions::Network);
    }

    UUID instanceUUID;
    deserializer >> instanceUUID;

    InboundResult result = {};
    type = doDeserializeEntityUpdate(context, deserializer, entity, {}, &result);

    if (type != EntityNetworkChanges::Type::Unknown || deserializer.getBytesLeft() != 0) {
        Logger::logDev("Not at end of entity network update byte stream, " +
            toString(deserializer.getBytesLeft()) + " bytes left", true);
    }

#if 0
    if (parentInstanceUUID.isValid()) {
        if (auto parentEntity = entity.getWorld().findEntity(parentInstanceUUID); parentEntity) {
            entity.setParent(parentEntity.value());
        } else {
            Logger::logError("Parent " + toString(parentInstanceUUID) + " not found for network entity \"" + entity.getName() + "\"");
        }
    }
#endif

    return result;
}

// See EntityFactory::updateEntityNode().
EntityNetworkChanges::Type EntityNetworkSerialize::doDeserializeEntityUpdate(
    const SerializationContext& context, Deserializer& deserializer,
    EntityRef& entity, const std::optional<EntityRef>& parent, InboundResult* result)
{
    Expects(entity.isValid());

    if (!entity.isSerializable()) {
        Logger::logDev("Rcv network update for non-serializable entity " + entity.getPrefabAssetId(), true);
    }

    if (parent) {
        entity.setParent(parent.value());
    }

    context.setCurrentEntity(entity);

    EntitySerializationContext serializationContext = {};
    serializationContext.resources = &session->getResources();
    serializationContext.entityContext = &context;
    serializationContext.interpolators = nullptr;
    serializationContext.entitySerializationTypeMask = makeMask(EntitySerialization::Type::Network);

    ByteSerializationContext byteSerializationContext = {};
    byteSerializationContext.resources = &session->getResources();
    byteSerializationContext.interpolators = session->getByteDataInterpolatorSet();
    byteSerializationContext.entityId = context.getCurrentEntityId();
    byteSerializationContext.entityInterpolators = context.getByteDataInterpolators();
    byteSerializationContext.entitySerializationContext = &serializationContext;

    // Entity
    uint8_t flags;
    deserializer >> flags;

    entity.setSelectable((flags & static_cast<uint8_t>(EntityData::Flag::NotSelectable)) == 0);
    entity.setSerializable((flags & static_cast<uint8_t>(EntityData::Flag::NotSerializable)) == 0);
    bool enabled = (flags & static_cast<uint8_t>(EntityData::Flag::Disabled)) == 0;

    // TODO: see EntityFactory::updateEntityNode()
    // - variants and rules

    entity.setEnabled(enabled);

    // EntityIdentity
    EntityNetworkChanges::Type type;
    uint32_t size;

    fetchNextPage(deserializer, type, size);

    if (type == EntityNetworkChanges::Type::EntityIdentity) {
        UUID parentInstanceUUID;
        deserializer >> parentInstanceUUID;

        UUID prefabUUID;
        deserializer >> prefabUUID;

        String name;
        deserializer >> name;
        entity.setName(name);

        //std::optional<String> prefabId;
        //deserializer >> prefabId;

        entity.setPrefab(prefabUUID.isValid() ? context.getPrefab() : std::shared_ptr<Prefab>(), prefabUUID);

        fetchNextPage(deserializer, type, size);
    }

    // Components
    while (type == EntityNetworkChanges::Type::Component) {
        uint16_t componentId;
        deserializer >> componentId;

        Expects(size > 0);

        const auto& reflector = deserializer.getOptions().world->getReflection().getComponentReflector(componentId);

        if (auto component = reflector.tryGetComponent(entity)) {
            if (result && componentId == Transform2DComponent::componentIndex) {
                // Saves the current position before deserialization, and restores it afterward. Return the
                // new position in the function result instead.
                //
                // Only done for transform components in root entities, skipped for child entities.
                //
                // TODO: for debugging, shouldn't be needed as it's updated later.
                auto transform = reinterpret_cast<Transform2DComponent*>(component);
                auto pos = transform->getLocalPosition();

                reflector.deserializeNetwork(byteSerializationContext, deserializer, *component);

                result->position = transform->getLocalPosition();
                transform->setLocalPosition(pos);
            } else {
                reflector.deserializeNetwork(byteSerializationContext, deserializer, *component);
            }
        } else {
            deserializer.skipBytes(size);
            Logger::logDev("No component " + toString(componentId) + " found in entity " +
                toString(entity.getEntityId().value & 0xffffffff) + ", " + entity.getName() + " to deserialize into, skip " + toString(size) + " bytes");
        }

        fetchNextPage(deserializer, type, size);
    }

    // Children
    while (type == EntityNetworkChanges::Type::Entity) {
        size_t marker = deserializer.getPosition();

        UUID childInstanceUUID;
        deserializer >> childInstanceUUID;

        auto childEntityInfo = findChildEntity(entity, childInstanceUUID);

        if (childEntityInfo) {
            auto& childEntity = childEntityInfo->first;
            const auto& parentOfChildEntity = childEntityInfo->second;
            type = doDeserializeEntityUpdate(context, deserializer, childEntity, parentOfChildEntity, nullptr);
        } else if (parent) {
            // Not a child entity, so it should be a sibling, or child of sibling, further up the
            // call chain. Need to rewind the deserializer, so the caller can read the UUIDs again.
            deserializer.rewind(marker);
            break;
        } else {
            // No child entity found, and we are at the root already. Something went wrong.
            // Let's skip this entity data and any components that follow.
            Logger::logWarning("Entity '" + childInstanceUUID + "' not found while deserializing update, skip " + toString(size) + " bytes");

            deserializer.rewind(marker);
            deserializer.skipBytes(size);

            fetchNextPage(deserializer, type, size);

            if (type == EntityNetworkChanges::Type::EntityIdentity) {
                deserializer.skipBytes(size);

                fetchNextPage(deserializer, type, size);
            }

            while (type == EntityNetworkChanges::Type::Component) {
                uint16_t componentId;
                deserializer >> componentId;

                deserializer.skipBytes(size);

                fetchNextPage(deserializer, type, size);
            }
        }
    }

    return type;
}

bool EntityNetworkSerialize::processEntityUpdateChanges(Bytes& previous)
{
    bool modified = previous.empty();
    hasComponentsAddedOrRemoved = false;

    // Compare with previously saved journal.

    if (!previous.empty()) {
        Deserializer s(previous);
        EntityNetworkChanges previousJournal;

        s >> previousJournal;

        // Fast check. We don't want to do the rather expensive work below if
        // nothing has changed since the last visit.

        modified = !(journal == previousJournal);

        if (modified) {
            // Something has changed. We need to do a more detailed inspection
            // to check for entity/component updates, additions and deletions.

            childrenAdded.clear();
            childrenChanged.clear();
            childrenRemoved.clear();

            // Enumerate all child entities in current journal. Mark all of
            // them as "potentially added".

            journal.enumerateEntities(
                    [this](const EntityNetworkChanges::Page& page, int pageIdx) {
                        if (pageIdx > 0) {
                            childrenAdded.emplace(page.uuid);
                        }
                    }
            );

            // Enumerate all child entities in previous journal. Compare with
            // current set to check which have been added, changed or removed.

            previousJournal.enumerateEntities(
                    [this](const EntityNetworkChanges::Page& page, int pageIdx) {
                        if (pageIdx > 0) {
                            if (childrenAdded.contains(page.uuid)) {
                                // Found in both - mark as "changed".
                                childrenAdded.erase(page.uuid);
                                childrenChanged.emplace(page.uuid);
                            } else {
                                // Not found in current journal - mark as "removed".
                                childrenRemoved.emplace(page.uuid);
                            }
                        }
                    }
            );

            // Enumerate again, but only care about entities marked as
            // "changed". This time include the root entity though.

            journal.enumerateEntityPages(
                    childrenChanged,
                    [&](const EntityNetworkChanges::Page& page, int pageIdx) {
                        // Search the matching page in previous journal.
                        int prevPageIdx;
                        auto& prevEntityPage = previousJournal.findEntityByUUID(page.uuid, prevPageIdx);

                        // Compare hashes for the entity page.
                        page.modified = page.hash != prevEntityPage.hash;

                        // Compare identity pages.
                        {
                            auto identityPage = journal.getEntityIdentity(pageIdx);
                            auto prevIdentityPage = previousJournal.getEntityIdentity(prevPageIdx);

                            identityPage->modified = identityPage->hash != prevIdentityPage->hash;

                            page.modified |= identityPage->modified;
                        }

                        // Enumerate components for this entity.
                        {
                            int componentPageIdx = pageIdx + 2;

                            while (auto componentPage = journal.findNextComponent(componentPageIdx)) {
                                // Search for the same component, by ID, in the previous journal.
                                EntityNetworkChanges::Page* prevComponentPage = nullptr;
                                int prevComponentPageIdx = prevPageIdx + 2;

                                while ((prevComponentPage = previousJournal.findNextComponent(prevComponentPageIdx)) != nullptr) {
                                    if (componentPage->componentId == prevComponentPage->componentId) {
                                        break;
                                    }
                                }

                                if (prevComponentPage != nullptr) {
                                    // Compare hashes of current and previous components.
                                    componentPage->modified = componentPage->hash != prevComponentPage->hash;

                                    // If any component has been modified, mark the entity page as modified too.
                                    page.modified |= componentPage->modified;
                                } else if (!session->allowComponentAddedForFastUpdate(componentPage->componentId)) {
                                    // Not found in previous journal.
                                    // Ask the listener, might want to ignore by component type.
                                    hasComponentsAddedOrRemoved = true;
                                }
                            }
                        }

                        // If no components have been added, compare the components again,
                        // but "reversed" - check for components that have been removed.

                        if (!hasComponentsAddedOrRemoved) {
                            int prevComponentPageIdx = prevPageIdx + 2;

                            while (auto prevComponentPage = previousJournal.findNextComponent(prevComponentPageIdx)) {
                                EntityNetworkChanges::Page* componentPage = nullptr;
                                int componentPageIdx = pageIdx + 2;

                                while ((componentPage = journal.findNextComponent(componentPageIdx)) != nullptr) {
                                    if (componentPage->componentId == prevComponentPage->componentId) {
                                        break;
                                    }
                                }

                                if (componentPage == nullptr && !session->allowComponentAddedForFastUpdate(prevComponentPage->componentId)) {
                                    hasComponentsAddedOrRemoved = true;
                                    break;
                                }
                            }
                        }
                    }
            );
        }
    }

    // Serialize & store the new journal.

    if (modified) {
        previous.resize_no_init(journal.getRequiredSerializeSize());

        Serializer s(previous.byte_span(), {});

        s << journal;

        previous.resize(s.getSize());
    }

    return modified;
}

bool EntityNetworkSerialize::hasEntityChanges(const EntityRef& entity, bool log) const
{
    bool changes = hasComponentsAddedOrRemoved;

    if (!childrenAdded.empty()) {
        if (log) {
            Logger::logDev("  - " + toString(childrenAdded.size()) + " children added");
            for (const auto& child : childrenAdded) {
                auto ce = findChildEntity(entity, child);
                Logger::logDev("    + " + child + " - " + (ce ? ce->first.getName() : "unknown"));
            }
        }
        changes = true;
    }

    if (!childrenRemoved.empty()) {
        if (log) {
            Logger::logDev("  - " + toString(childrenRemoved.size()) + " children removed");
        }
        changes = true;
    }

    return changes;
}

size_t EntityNetworkSerialize::getBytes(Bytes& data, const SerializerOptions& options, bool log) const
{
    data.resize_no_init(data.capacity());

    SerializerOptions opt(SerializerOptions::maxVersion);
    opt.dictionary = options.dictionary;

    Serializer s(data.byte_span(), opt);
    journal.writeJournal(s, scratchpad, log);

    return s.getSize();
}

size_t EntityNetworkSerialize::getBytesCapacity()
{
    return scratchpad.capacity();
}

void EntityNetworkSerialize::fetchNextPage(Deserializer& deserializer, EntityNetworkChanges::Type& type, uint32_t& size)
{
    type = EntityNetworkChanges::Type::Unknown;
    size = 0;

    if (deserializer.getBytesLeft() > 0) {
        checkMagic(deserializer, SERIALIZE_CHECK_PARITY);

        uint8_t t;
        deserializer >> t;
        type = static_cast<EntityNetworkChanges::Type>(t);
    }

    // Only fetch size for page types which actually contain data; keep size=0
    // for everything else.

    if (type == EntityNetworkChanges::Type::Entity ||
        type == EntityNetworkChanges::Type::EntityIdentity ||
        type == EntityNetworkChanges::Type::Component) {

        if (deserializer.getBytesLeft() > 0) {
            uint32_t s;
            deserializer >> s;
            size = s;
        }
    } else if (type != EntityNetworkChanges::Type::Unknown) {
        throw Exception("Not a valid entity change page", HalleyExceptions::Network);
    }
}

std::optional<std::pair<EntityRef, EntityRef>> EntityNetworkSerialize::findChildEntity(const EntityRef& entity, const UUID& instanceUUID)
{
    for (auto child : entity.getChildren()) {
        if (child.getInstanceUUID() == instanceUUID) {
            return std::make_pair(child, entity);
        }
        if (const auto descend = findChildEntity(child, instanceUUID)) {
            return descend;
        }
    }

    return {};
}
