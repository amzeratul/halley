#include "halley/net/entity/entity_network_serialize.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/entity/world.h"

using namespace Halley;

#define SERIALIZE_PARITY_BYTE 0xae

thread_local Bytes EntityNetworkSerialize::scratchpad;

void EntityNetworkChanges::beginPage(Serializer& serializer, Type type)
{
    curPage.hash = 0;
    curPage.from = (uint16_t) serializer.getPosition();
    curPage.to = curPage.from;

    curPage.type = type;

    curPage.modified = true;
}

void EntityNetworkChanges::endPage(Serializer& serializer, Bytes& buffer, Type type)
{
    curPage.to = (uint16_t) serializer.getPosition();

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
    if (!entity.isSelectable()) flags |= (uint8_t) EntityData::Flag::NotSelectable;
    if (!entity.isSerializable()) flags |= (uint8_t) EntityData::Flag::NotSerializable;
    if (!entity.isEnabled()) flags |= (uint8_t) EntityData::Flag::Disabled;

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

    const auto& prefabId = entity.getPrefabAssetId().value_or("");
    serializer << prefabId;

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

#ifdef SERIALIZE_PARITY_BYTE
            serializer << (uint8_t) SERIALIZE_PARITY_BYTE;
#endif

            serializer << (uint8_t) page.type;
            serializer << (uint16_t) size;

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

EntityNetworkSerialize::EntityNetworkSerialize(Resources& resources, const IByteDataInterpolatorSet* interpolators)
    : resources(resources)
    , interpolators(interpolators)
    , hasComponentsAddedOrRemoved(false)
{
    scratchpad.reserve(16384);
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
    serializationContext.resources = &resources;
    serializationContext.entityContext = &context;
    serializationContext.interpolators = nullptr;
    serializationContext.entitySerializationTypeMask = makeMask(EntitySerialization::Type::Network);

    ByteSerializationContext byteSerializationContext = {};
    byteSerializationContext.resources = &resources;
    byteSerializationContext.interpolators = interpolators;
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

void EntityNetworkSerialize::deserializeEntityUpdate(EntityRef& entity, const std::shared_ptr<const Prefab>& prefab, const Bytes& bytes, const SerializerOptions& options)
{
    SerializerOptions opt(SerializerOptions::maxVersion);
    opt.dictionary = options.dictionary;
    opt.world = &entity.getWorld();

    Deserializer deserializer(bytes, opt);
    const SerializationContext context(entity, prefab);

    EntityNetworkChanges::Type type;
    uint16_t size;
    fetchNextPage(deserializer, type, size);

    if (type != EntityNetworkChanges::Type::Entity) {
        throw Exception("Unexpected entity network change type", HalleyExceptions::Network);
    }

    UUID instanceUUID;
    deserializer >> instanceUUID;

    type = doDeserializeEntityUpdate(context, deserializer, entity, {});

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
}

// See EntityFactory::updateEntityNode().
EntityNetworkChanges::Type EntityNetworkSerialize::doDeserializeEntityUpdate(
    const SerializationContext& context, Deserializer& deserializer,
    EntityRef& entity, const std::optional<EntityRef>& parent)
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
    serializationContext.resources = &resources;
    serializationContext.entityContext = &context;
    serializationContext.interpolators = nullptr;
    serializationContext.entitySerializationTypeMask = makeMask(EntitySerialization::Type::Network);

    ByteSerializationContext byteSerializationContext = {};
    byteSerializationContext.resources = &resources;
    byteSerializationContext.interpolators = interpolators;
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
    uint16_t size;

    fetchNextPage(deserializer, type, size);

    if (type == EntityNetworkChanges::Type::EntityIdentity) {
        UUID parentInstanceUUID;
        deserializer >> parentInstanceUUID;

        UUID prefabUUID;
        deserializer >> prefabUUID;

        String name;
        deserializer >> name;
        entity.setName(name);

        String prefabId;
        deserializer >> prefabId;

        entity.setPrefab(prefabUUID.isValid() ? context.getPrefab() : std::shared_ptr<Prefab>(), prefabUUID);

        fetchNextPage(deserializer, type, size);
    }

    // Components
    while (type == EntityNetworkChanges::Type::Component) {
        uint16_t componentId;
        deserializer >> componentId;

        if (size > 0) {
            const auto& reflector = deserializer.getOptions().world->getReflection().getComponentReflector(componentId);

            if (auto component = reflector.tryGetComponent(entity)) {
                reflector.deserializeNetwork(byteSerializationContext, deserializer, *component);
            } else {
                deserializer.skipBytes(size);
                Logger::logDev("No component " + toString(componentId) + " found in entity " + entity.getName() + " to deserialize into, skip " + toString(size) + " bytes");
            }
        }

        fetchNextPage(deserializer, type, size);
    }

    // Children
    while (type == EntityNetworkChanges::Type::Entity) {
        size_t marker = deserializer.getPosition();

        UUID childInstanceUUID;
        deserializer >> childInstanceUUID;

        auto childEntity = findChildEntity(entity, childInstanceUUID);

        if (childEntity) {
            type = doDeserializeEntityUpdate(context, deserializer, childEntity.value(), entity);
        } else if (parent) {
            // Not a child entity, so it should be a sibling, or child of sibling, further up the
            // call chain. Need to rewind the deserializer, so the caller can read the UUIDs again.
            deserializer.rewind(marker);
            break;
        } else {
            // No child entity found, and we are at the root already. Something went wrong.
            // Let's skip this entity data and any components that follow.
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
                                } else {
                                    // not found in previous journal
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

                                if (componentPage == nullptr) {
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
                Logger::logDev("    + " + child + " - " + (ce ? ce->getName() : "unknown"));
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

void EntityNetworkSerialize::getBytes(Bytes& data, const SerializerOptions& options, bool log) const
{
    data.resize_no_init(data.capacity());

    SerializerOptions opt(SerializerOptions::maxVersion);
    opt.dictionary = options.dictionary;

    Serializer s(data.byte_span(), opt);
    journal.writeJournal(s, scratchpad, log);

    data.resize(s.getSize());
}

void EntityNetworkSerialize::fetchNextPage(Deserializer& deserializer, EntityNetworkChanges::Type& type, uint16_t& size)
{
    type = EntityNetworkChanges::Type::Unknown;
    size = 0;

    if (deserializer.getBytesLeft() > 0) {
#ifdef SERIALIZE_PARITY_BYTE
        uint8_t parity;
        deserializer >> parity;
        if (parity != SERIALIZE_PARITY_BYTE) {
            throw Exception("Not a valid entity change page", HalleyExceptions::Network);
        }
#endif

        uint8_t t;
        deserializer >> t;
        type = (EntityNetworkChanges::Type) t;
    }

    // Only fetch size for page types which actually contain data; keep size=0
    // for everything else.

    if (type == EntityNetworkChanges::Type::Entity ||
        type == EntityNetworkChanges::Type::EntityIdentity ||
        type == EntityNetworkChanges::Type::Component) {

        if (deserializer.getBytesLeft() > 0) {
            uint16_t s;
            deserializer >> s;
            size = s;
        }
    } else if (type != EntityNetworkChanges::Type::Unknown) {
        throw Exception("Not a valid entity change page", HalleyExceptions::Network);
    }
}

std::optional<EntityRef> EntityNetworkSerialize::findChildEntity(const EntityRef& entity, const UUID& instanceUUID)
{
    for (auto child : entity.getChildren()) {
        if (child.getInstanceUUID() == instanceUUID) {
            return child;
        }
        if (const auto descend = findChildEntity(child, instanceUUID)) {
            return descend;
        }
    }

    return {};
}
