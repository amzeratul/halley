#include "halley/net/entity/entity_network_serialize.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/entity/world.h"
#include "halley/entity/components/transform_2d_component.h"

using namespace Halley;

#define INJECT_RUNTIME_CHECKS DEV_BUILD
static constexpr uint16_t SERIALIZE_CHECK_PARITY = 0xbaad;

static void injectMagic(Serializer& serializer, uint16_t n)
{
    serializer << gsl::span(reinterpret_cast<std::byte *>(&n), sizeof(n));
}

static bool checkMagic(Deserializer& deserializer, uint16_t n, bool throwOnError)
{
    uint16_t m;
    deserializer >> gsl::span(reinterpret_cast<std::byte *>(&m), sizeof(m));

    if (m != n && throwOnError) {
        throw Exception("Network stream error", HalleyExceptions::Network);
    }

    return m == n;
}

static void injectSimpleStringChecksum(Serializer& serializer, const std::string_view& str)
{
    size_t m = str.length();
    for (const char ch : str) {
        m += ch;
    }
    injectMagic(serializer, m & 0xffff);
}

static bool checkSimpleStringChecksum(Deserializer& deserializer, const std::string_view& str)
{
    size_t m = str.length();
    for (const char ch : str) {
        m += ch;
    }
    return checkMagic(deserializer, m & 0xffff, false);
}

thread_local Bytes EntityNetworkSerialize::scratchpad;

void EntityNetworkChanges::beginPage(Serializer& serializer, Type type)
{
    memset(&curPage, 0, sizeof(curPage));

    curPage.from = static_cast<uint32_t>(serializer.getPosition());
    curPage.to = curPage.from;

    curPage.type = type;

    curPage.modified = true;
}

void EntityNetworkChanges::endPage(Serializer& serializer, Bytes& buffer, Type type)
{
    curPage.to = static_cast<uint32_t>(serializer.getPosition());

    HalleyAssertDev(curPage.type == type);
    HalleyAssertDev(curPage.to >= curPage.from);

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

void EntityNetworkChanges::pushEntity(Serializer& serializer, const EntityRef& entity, bool remote, const std::optional<EntityRef>& parent, Bytes& buffer)
{
    // Entity "header"
    beginPage(serializer, Type::Entity);
    curPage.uuid = entity.getInstanceUUID();
    curPage.remote = remote;

    serializer << entity.getInstanceUUID();

    uint8_t flags = 0;
    if (!entity.isSelectable()) flags |= static_cast<uint8_t>(EntityData::Flag::NotSelectable);
    if (!entity.isSerializable()) flags |= static_cast<uint8_t>(EntityData::Flag::NotSerializable);
    if (!entity.isEnabled()) flags |= static_cast<uint8_t>(EntityData::Flag::Disabled);

    serializer << flags;

    endPage(serializer, buffer, Type::Entity);

    // Identity
    beginPage(serializer, Type::EntityIdentity);

    UUID parentInstanceUUID = {};
    if (parent.has_value()) {
        parentInstanceUUID = parent->getInstanceUUID();
    }

    serializer << parentInstanceUUID;
    serializer << entity.getPrefabUUID();

#if INJECT_RUNTIME_CHECKS
    injectSimpleStringChecksum(serializer, entity.getName());
#endif

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
    s << static_cast<uint16_t>(pp);

    for (int p = 0; p < pp; p++) {
        auto& page = pages[p];
        s << page.hash;
        s << page.from;
        s << page.to;
        s << page.type;
        if (page.type == Type::Entity) {
            s << page.uuid;
            s << page.remote;
        } else if (page.type == Type::Component) {
            s << page.componentId;
        }
    }
}

void EntityNetworkChanges::deserialize(Deserializer& s)
{
    uint16_t count;
    s >> count;
    pp = count;

    memset(&pages[0], 0, pp * sizeof(Page));

    contentHasher.reset();

    for (int p = 0; p < pp; p++) {
        auto& page = pages[p];
        s >> page.hash;
        s >> page.from;
        s >> page.to;
        s >> page.type;
        if (page.type == Type::Entity) {
            s >> page.uuid;
            s >> page.remote;
        } else if (page.type == Type::Component) {
            s >> page.componentId;
        }

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
        HalleyAssertDev(contentHash != 0);
        HalleyAssertDev(other.contentHash != 0);
        eq = contentHash == other.contentHash;
    }

    return eq;
}

// Parses the journal, along with the entity data, and writes everything to the
// output buffer. Tries to skip unmodified data along the way.
void EntityNetworkChanges::writeJournal(Serializer& serializer, const Bytes& buffer, bool log) const
{
    HalleyAssertDev(pp > 0);

    int p = 0;
    while (p < pp) {
        auto& page = pages[p];

        bool skip = false;
        size_t size = page.to - page.from;

        if (page.type == Type::Entity) {
            // The check for page 0 is important here - we don't want to skip the root entity!
            if (p > 0 && !page.modified) {
                skip = true;

                // Seek forward to the next entity.
                int pn = p + 1;
                while (pn < pp) {
                    auto& next = pages[pn];
                    if (next.type == Type::Entity) {
                        break;
                    }
                    if (next.modified) {
                        // TODO: This shouldn't happen, actually - but it does?
                        // Kind of auto-correct here: do not skip.
                        skip = false;
                        break;
                    }
                    pn++;
                }

                if (skip) {
                    p = pn;
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
            HalleyAssertDev(page.type == Type::Entity ||
                page.type == Type::EntityIdentity ||
                page.type == Type::Component);

#if INJECT_RUNTIME_CHECKS
            injectMagic(serializer, SERIALIZE_CHECK_PARITY);
#endif

            serializer << static_cast<uint8_t>(page.type);
            serializer << static_cast<uint32_t>(size);

            if (page.type == Type::Component) {
                // "Inject" component ID
                serializer << page.componentId;
            }

            HalleyAssertDev(size > 0);
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
    HalleyAssertDev(pageIdx + 1 < pp);
    HalleyAssertDev(pages[pageIdx].type == Type::Entity);
    HalleyAssertDev(pages[pageIdx + 1].type == Type::EntityIdentity);

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

void EntityNetworkChanges::invalidateHashes()
{
    for (int p = 0; p < pp; p++) {
        auto& page = pages[p];
        if (page.type == Type::Component) {
            page.hash = 0;
        }
    }
}

EntityNetworkSerialize::EntityNetworkSerialize(const EntityNetworkSession* session, EntityRef& entity)
    : session(session)
    , rootEntity(entity)
    , hasComponentsAddedOrRemoved(false)
{
    myPeerId = session->getSession().getMyPeerId().value_or(0);

    // Thread-local buffer to serialize/encode changes into.
    // The maximum size depends on some limits: MTU, maximum UDP packet split size in AckUnreliableConnection,
    // and maximum network message split size in EntityNetworkSession.
    scratchpad.reserve(16 * 32 * 1024);
    scratchpad.resize_no_init(scratchpad.capacity());

    // We lookup components by ID, need to translate from names.
    // This is the same list of component types EntityFactory uses to compile deltas.
    const auto& reflection = entity.getWorld().getReflection();
    const auto& ignoreComponents = session->getEntityDeltaOptions().ignoreComponents;
    componentsIgnored.reserve(ignoreComponents.size());
    for (const auto& componentName : ignoreComponents) {
        const auto& reflector = reflection.getComponentReflector(componentName);
        componentsIgnored.emplace(reflector.getIndex());
    }
}

bool EntityNetworkSerialize::serializeEntityUpdate(const SerializerOptions& options)
{
    SerializerOptions opt(SerializerOptions::maxVersion);
    opt.dictionary = options.dictionary;
    opt.world = &rootEntity.getWorld();

    Serializer serializer(scratchpad.byte_span(), opt);
    const SerializationContext context(rootEntity);

    doSerializeEntityUpdate(context, serializer, rootEntity, false, {});

    journal.digest();

    return !journal.isFull();
}

void EntityNetworkSerialize::doSerializeEntityUpdate(
    const SerializationContext& context, Serializer& serializer,
    const EntityRef& entity, bool remote, const std::optional<EntityRef>& parent)
{
#if INJECT_RUNTIME_CHECKS
    if (!entity.isSerializable()) {
        Logger::logDev("Send network update for non-serializable entity " + entity.getPrefabAssetId(), true);
    }
#endif

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

    // Network components needs some special treatment.
    // - Let only the host ever send NetworkComponent data.
    // - For *child* entities, if someone else grabbed authority, do not serialize any
    //   component data at all, for this child and all its sub-children.
    const bool canSerializeNetworkComponent = session->isHost();
    if (parent && !remote) {
        if (const auto networkComponent = entity.tryGetComponent<NetworkComponent>()) {
            if (networkComponent->authorityId.has_value()) {
                remote = networkComponent->authorityId != myPeerId;
            }
        }
    }

    // Entity
    journal.pushEntity(serializer, entity, remote, parent, scratchpad);

    // Components
    if (!remote) {
        auto& reflection = serializer.getOptions().world->getReflection();

        for (auto [componentId, component] : entity) {
            if (!canSerializeNetworkComponent && componentId == NetworkComponent::componentIndex) {
                continue;
            }

            if (componentsIgnored.contains(componentId)) {
                continue;
            }

            const auto& reflector = reflection.getComponentReflector(componentId);

            journal.beginComponent(serializer, static_cast<uint16_t>(componentId));
            reflector.serializeNetwork(byteSerializationContext, serializer, *component);
            journal.endComponent(serializer, scratchpad);
        }
    }

    // Children
    for (const auto& child : entity.getChildren()) {
        if (!child.isSerializable()) {
            continue;
        }
        doSerializeEntityUpdate(context, serializer, child, remote, entity);
    }
}

EntityNetworkSerialize::InboundResult EntityNetworkSerialize::deserializeEntityUpdate(const Bytes& bytes, const SerializerOptions& options)
{
    SerializerOptions opt(SerializerOptions::maxVersion);
    opt.dictionary = options.dictionary;
    opt.world = &rootEntity.getWorld();

    Deserializer deserializer(bytes, opt);
    const SerializationContext context(rootEntity, rootEntity.getPrefab());

    EntityNetworkChanges::Type type;
    uint32_t size;
    fetchNextPage(deserializer, type, size);

    if (type != EntityNetworkChanges::Type::Entity) {
        throw Exception("Unexpected entity network change type", HalleyExceptions::Network);
    }

    UUID instanceUUID;
    deserializer >> instanceUUID;

    InboundResult result = {};
    type = doDeserializeEntityUpdate(context, deserializer, rootEntity, {}, &result);

    if (type != EntityNetworkChanges::Type::Unknown || deserializer.getBytesLeft() != 0) {
        Logger::logDev("Not at end of entity network update byte stream, " +
            toString(deserializer.getBytesLeft()) + " bytes left", true);
    }

    return result;
}

// See EntityFactory::updateEntityNode().
EntityNetworkChanges::Type EntityNetworkSerialize::doDeserializeEntityUpdate(
    const SerializationContext& context, Deserializer& deserializer,
    EntityRef& entity, const std::optional<EntityRef>& parent, InboundResult* result)
{
    HalleyAssertDev(entity.isValid());

#if INJECT_RUNTIME_CHECKS
    if (!entity.isSerializable()) {
        Logger::logDev("Rcv network update for non-serializable entity " + entity.getPrefabAssetId(), true);
    }
#endif

    if (const auto networkComponent = entity.tryGetComponent<NetworkComponent>()) {
        if (networkComponent->authorityId == myPeerId) {
            Logger::logError("Rcv network update for entity " + entity.getEntityId().toDetailedString() + ", but have authority", true);
        }
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

#if INJECT_RUNTIME_CHECKS
        if (!checkSimpleStringChecksum(deserializer, entity.getName())) {
            // NB: right now this is expected to happen, names are not consistently
            // serialized for entity hierarchies - just report, don't throw exception!
            Logger::logDev("Entity name mismatch: " + entity.getName() +
                " differs from checksum sent, " + entity.getInstanceUUID(), true);
        }
#endif

        if (prefabUUID.isValid() && prefabUUID != entity.getPrefabUUID()) {
            throw Exception("Prefab UUID mismatch", HalleyExceptions::Network);
        }

        fetchNextPage(deserializer, type, size);
    }

    // Components
    while (type == EntityNetworkChanges::Type::Component) {
        uint16_t componentId;
        deserializer >> componentId;

        HalleyAssertDev(size > 0);

        const auto* reflector = deserializer.getOptions().world->getReflection().tryGetComponentReflector(componentId);

        if (reflector != nullptr) {
            // This checks with evenIfDisabled = true; the entity or its parent could have been
            // disabled while a network update is still in flight.
            auto component = reflector->tryGetComponent(entity, true);

            if (component == nullptr && session->allowComponentAddedForFastUpdate(componentId)) {
                // Create this component on demand if it's in the list of component types that are
                // allowed to be added (on sender side) without triggering the "slow update" path.
                //
                // This doesn't honor per-entity byte data interpolators. As those are only used for
                // the sending party, this would only be needed if ..
                // - a component type with a per-entity interpolator is added here
                // - a peer grabs authority of that entity later, flipping sender/receiver
                component = reflector->createComponent(entity);
            }

            if (component != nullptr) {
                const size_t expectedEndPos = deserializer.getPosition() + size;

                // Only done for transform components in root entities, skipped for child entities.
                if (result && componentId == Transform2DComponent::componentIndex) {
                    // Check if position interpolation is enabled.
                    // TODO: This lookup is kind of costly, as it is done again in deserializeNetwork().
                    const auto* interpolator = byteSerializationContext.entityInterpolators->tryGetInterpolator(
                        byteSerializationContext.entityId, componentId, "position");

                    if (interpolator && interpolator->isEnabled()) {
                        // Saves the current position before deserialization, and restores it afterward. Return the
                        // new position in the function result instead.
                        // TODO: for debugging, shouldn't be needed as it's updated later.
                        const auto transform = reinterpret_cast<Transform2DComponent*>(component);
                        const auto pos = transform->getLocalPosition();

                        reflector->deserializeNetwork(byteSerializationContext, deserializer, *component);

                        result->position = transform->getLocalPosition();
                        transform->setLocalPosition(pos);
                    } else {
                        reflector->deserializeNetwork(byteSerializationContext, deserializer, *component);
                    }
                } else {
                    reflector->deserializeNetwork(byteSerializationContext, deserializer, *component);
                }

                if (expectedEndPos < deserializer.getPosition()) {
                    Logger::logError("Network read error, read past end of component " + toString(componentId) + ", rewind " +
                        toString(deserializer.getPosition() - expectedEndPos) + " bytes", true);
                    deserializer.rewind(expectedEndPos);
                } else if (expectedEndPos > deserializer.getPosition()) {
                    Logger::logError("Network read error, component " + toString(componentId) + " not fully read, skip " +
                        toString(expectedEndPos - deserializer.getPosition()) + " bytes", true);
                    deserializer.skipBytes(expectedEndPos - deserializer.getPosition());
                }
            } else {
                deserializer.skipBytes(size);
                Logger::logDev("No component " + toString(componentId) + " found in entity " +
                    toString(entity.getEntityId().value & 0xffffffff) + ", " + entity.getName() + " to deserialize into, skip " + toString(size) + " bytes");
            }
        } else {
            deserializer.skipBytes(size);
            Logger::logDev("Invalid component type " + toString(componentId) + " found for " +
                entity.getName() + " to deserialize into, skip " + toString(size) + " bytes");
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
            uint32_t totalSkipSize = size;

            deserializer.rewind(marker);
            deserializer.skipBytes(size);

            fetchNextPage(deserializer, type, size);

            if (type == EntityNetworkChanges::Type::EntityIdentity) {
                deserializer.skipBytes(size);
                totalSkipSize += size;

                fetchNextPage(deserializer, type, size);
            }

            while (type == EntityNetworkChanges::Type::Component) {
                uint16_t componentId;
                deserializer >> componentId;

                deserializer.skipBytes(size);
                totalSkipSize += size;

                fetchNextPage(deserializer, type, size);
            }

            Logger::logWarning("Child entity '" + childInstanceUUID + "' not found as child of " +
                entity.getEntityId().toDetailedString() + " for deserializing update, skipped " + toString(totalSkipSize) + " bytes", true);
        }
    }

    return type;
}

bool EntityNetworkSerialize::processEntityUpdateChanges(Bytes& previous, bool sendFullUpdate)
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

        if (sendFullUpdate) {
            previousJournal.invalidateHashes();
            modified = true;
        } else {
            modified = journal != previousJournal;
        }

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
                        page.modified = !page.remote && page.hash != prevEntityPage.hash;

                        // Compare identity pages.
                        {
                            auto identityPage = journal.getEntityIdentity(pageIdx);
                            auto prevIdentityPage = previousJournal.getEntityIdentity(prevPageIdx);

                            identityPage->modified = !page.remote && identityPage->hash != prevIdentityPage->hash;

                            page.modified |= identityPage->modified;
                        }

                        // Enumerate components for this entity.
                        // Can be skipped for "remote" pages.
                        if (!page.remote) {
                            int componentPageIdx = pageIdx + 2;

                            while (auto componentPage = journal.findNextComponent(componentPageIdx)) {
                                HalleyAssertDebug(!page.remote);

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

                        if (!page.remote && !hasComponentsAddedOrRemoved) {
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

                        // Sanity check: remote pages *must not* be marked as modified!
                        HalleyAssertDebug(!(page.modified && page.remote));
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

bool EntityNetworkSerialize::hasEntityChanges(bool log) const
{
    bool changes = hasComponentsAddedOrRemoved;

    if (!childrenAdded.empty()) {
        if (log) {
            Logger::logDev("  - " + toString(childrenAdded.size()) + " children added");
            for (const auto& child : childrenAdded) {
                auto ce = findChildEntity(rootEntity, child);
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
    {
        EntityNetworkChanges::Type nextType = EntityNetworkChanges::Type::Unknown;

        if (deserializer.getBytesLeft() > 0) {
#if INJECT_RUNTIME_CHECKS
            checkMagic(deserializer, SERIALIZE_CHECK_PARITY, true);
#endif

            uint8_t t;
            deserializer >> t;
            nextType = static_cast<EntityNetworkChanges::Type>(t);
        }

        type = nextType;
    }

    // Only fetch size for page types which actually contain data; keep size=0
    // for everything else.

    size = 0;

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
