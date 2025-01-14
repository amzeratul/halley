#include "halley/net/entity/entity_network_serialize.h"

#include "halley/bytes/byte_serializer.h"
#include "halley/entity/world.h"

using namespace Halley;

thread_local Bytes EntityNetworkSerialize::scratchpad;

thread_local HashSet<UUID> EntityNetworkSerialize::childrenAdded;
thread_local HashSet<UUID> EntityNetworkSerialize::childrenChanged;
thread_local HashSet<UUID> EntityNetworkSerialize::childrenRemoved;

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

void EntityNetworkChanges::beginEntity(Serializer& serializer, const EntityRef& entity, std::optional<EntityRef> parent)
{
    beginPage(serializer, Type::Entity);
    curPage.uuid = entity.getInstanceUUID();

    bool isRootEntity = !parent.has_value();
    serializer << isRootEntity;

    UUID parentUUID;
    if (isRootEntity) {
        if (auto parentEntity = entity.tryGetParent()) {
            parentUUID = parentEntity->getInstanceUUID();
        }
    } else {
        parentUUID = parent->getInstanceUUID();
    }

    serializer << entity.getInstanceUUID();
    serializer << parentUUID;

    uint8_t flags = 0;
    if (!entity.isSelectable()) flags |= (uint8_t) EntityData::Flag::NotSelectable;
    if (!entity.isEnabled()) flags |= (uint8_t) EntityData::Flag::Disabled;

    serializer << entity.getName();
    serializer << flags;
    serializer << entity.getPrefabUUID();
}

void EntityNetworkChanges::endEntity(Serializer& serializer, Bytes& buffer)
{
    endPage(serializer, buffer, Type::Entity);
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
void EntityNetworkChanges::writeJournal(Serializer& serializer, const Bytes& buffer) const
{
    Expects(pp > 0);

    int p = 0;
    while (p < pp) {
        auto& page = pages[p];

        bool skip = false;
        size_t size = page.to - page.from;

        if (page.type == Type::Entity) {
            if (!page.modified) {
                skip = true;
                p++;

                // Seek forward until end of this entity, including its components.

                while (p < pp) {
                    auto& next = pages[p];
                    if (next.type != Type::Component) {
                        break;
                    }
                    Ensures(!next.modified);
                    p++;
                }
            }
        } else if (page.type == Type::Component) {
            if (!page.modified || size == 0) {
                skip = true;
                p++;
            }
        }

        if (!skip) {
            Expects(page.type == Type::Entity ||
                page.type == Type::Component);

            serializer << (uint8_t) page.type;
            serializer << (uint16_t) size;

            if (page.type == Type::Component) {
                // "Inject" component ID
                serializer << (uint16_t) page.componentId;
            }

            Expects(size > 0);
            auto span = buffer.const_byte_span().subspan(page.from, size);
            serializer << span;

            p++;
        }
    }
}

void EntityNetworkChanges::enumerateEntityPages(const HashSet<UUID>& filter,
        const std::function<void (const Page& page, int pageIdx)>& onEntity) const
{
    enumerateEntities(
            [&filter, onEntity](const Page &page, int pageIdx) {
                if (filter.contains(page.uuid)) {
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

EntityNetworkSerialize::EntityNetworkSerialize(Resources& resources)
    : resources(resources)
    , journal()
{
    scratchpad.reserve(8192);
    scratchpad.resize_no_init(scratchpad.capacity());
}

bool EntityNetworkSerialize::serializeEntityUpdate(const EntityRef& entity, const SerializerOptions& options)
{
    SerializerOptions opt(SerializerOptions::maxVersion);
    opt.dictionary = options.dictionary;
    opt.world = &entity.getWorld();

    Serializer serializer(scratchpad.byte_span(), opt);

    doSerializeEntityUpdate(serializer, entity, {});

    journal.digest();

    return !journal.isFull();
}

void EntityNetworkSerialize::doSerializeEntityUpdate(Serializer& serializer, const EntityRef& entity, std::optional<EntityRef> parent)
{
    if (!entity.isSerializable()) {
        Logger::logError("Send network update for non-serializable entity " + entity.getPrefabAssetId(), true);
    }

    EntitySerializationContext serializationContext = {};
    serializationContext.resources = &resources;
    serializationContext.entitySerializationTypeMask = makeMask(EntitySerialization::Type::Network);

    // Entity
    journal.beginEntity(serializer, entity, parent);
    journal.endEntity(serializer, scratchpad);

    // Components
    auto& reflection = serializer.getOptions().world->getReflection();

    for (auto [componentId, component] : entity) {
        const auto& reflector = reflection.getComponentReflector(componentId);

        journal.beginComponent(serializer, (uint16_t) componentId);
        reflector.serializeNetwork(serializationContext, serializer, *component);
        journal.endComponent(serializer, scratchpad);
    }

    // Children
    for (const auto& child : entity.getChildren()) {
        if (!child.isSerializable()) {
            continue;
        }
        doSerializeEntityUpdate(serializer, child, entity);
    }
}

void EntityNetworkSerialize::deserializeEntityUpdate(EntityRef& entity, const Bytes& bytes, const SerializerOptions& options, const std::shared_ptr<EntityFactoryContext>& context)
{
    SerializerOptions opt(SerializerOptions::maxVersion);
    opt.dictionary = options.dictionary;
    opt.world = &entity.getWorld();

    Deserializer deserializer(bytes, opt);

    EntityNetworkChanges::Type type;
    uint16_t size;

    fetchNextPage(deserializer, type, size);

    while (type != EntityNetworkChanges::Type::Unknown) {
        if (type != EntityNetworkChanges::Type::Entity) {
            throw Exception("Unexpected entity network change type", HalleyExceptions::Network);
        }

        bool isRootEntity;
        deserializer >> isRootEntity;

        UUID instanceUUID, parentInstanceUUID;
        deserializer >> instanceUUID;
        deserializer >> parentInstanceUUID;

        std::optional<EntityRef> parentEntity;
        std::optional<EntityRef> childEntity;

        if (isRootEntity) {
            childEntity = entity;
            Expects(entity.getInstanceUUID() == instanceUUID);
        } else {
            childEntity = opt.world->findEntity(instanceUUID);
        }

        type = doDeserializeEntityUpdate(deserializer, childEntity.value(), parentEntity, context);

        if (isRootEntity && parentInstanceUUID.isValid()) {
            if (auto p = opt.world->findEntity(parentInstanceUUID)) {
                entity.setParent(p.value());
            }
        }
    }

    if (deserializer.getBytesLeft() != 0) {
        throw Exception("Not at end of entity network update byte stream", HalleyExceptions::Network);
    }
}

EntityNetworkChanges::Type EntityNetworkSerialize::doDeserializeEntityUpdate(Deserializer& deserializer, EntityRef& entity, std::optional<EntityRef> parent, const std::shared_ptr<EntityFactoryContext>& context)
{
    Expects(entity.isValid());
    Expects(entity.isSerializable());

    EntitySerializationContext serializationContext = {};
    serializationContext.resources = &resources;
    serializationContext.entitySerializationTypeMask = makeMask(EntitySerialization::Type::Network);

    context->setCurrentEntity(entity.getEntityId());

    {
        String name;
        deserializer >> name;

        entity.setName(name);

        uint8_t flags;
        deserializer >> flags;

        entity.setSelectable((flags & static_cast<uint8_t>(EntityData::Flag::NotSelectable)) == 0);

        bool enabled = (flags & static_cast<uint8_t>(EntityData::Flag::Disabled)) == 0;
        entity.setEnabled(enabled);

        // TODO: see EntityFactory::updateEntityNode()
        // - variants and rules

        UUID prefabUUID;
        deserializer >> prefabUUID;

        entity.setPrefab(prefabUUID.isValid() ? context->getPrefab() : std::shared_ptr<Prefab>(), prefabUUID);
    }

    EntityNetworkChanges::Type type;
    uint16_t size;

    fetchNextPage(deserializer, type, size);

    while (type == EntityNetworkChanges::Type::Component) {
        uint16_t componentId, componentSize;

        deserializer >> componentId;
        componentSize = size;

        const auto& reflector = deserializer.getOptions().world->getReflection().getComponentReflector(componentId);

        if (auto component = reflector.tryGetComponent(entity)) {
            reflector.deserializeNetwork(serializationContext, deserializer, *component);
        } else {
            // TODO:
            if (componentSize > 0) {
                deserializer.skipBytes(componentSize);
            }
        }

        fetchNextPage(deserializer, type, size);
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
                    [](const EntityNetworkChanges::Page& page, int pageIdx) {
                        if (pageIdx > 0) {
                            childrenAdded.emplace(page.uuid);
                        }
                    }
            );

            // Enumerate all child entities in previous journal. Compare with
            // current set to check which have been added, changed or removed.

            previousJournal.enumerateEntities(
                    [](const EntityNetworkChanges::Page& page, int pageIdx) {
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
            // "changed". This includes to root entity though.

            journal.enumerateEntityPages(
                    childrenChanged,
                    [&](const EntityNetworkChanges::Page& page, int pageIdx) {
                        // Search the matching page in previous journal.
                        int prevPageIdx;
                        auto& prevEntityPage = previousJournal.findEntityByUUID(page.uuid, prevPageIdx);

                        // Compare hashes for the entity page.
                        page.modified = page.hash != prevEntityPage.hash;

                        // Enumerate components for this entity.
                        {
                            int componentPageIdx = pageIdx + 1;

                            while (auto componentPage = journal.findNextComponent(componentPageIdx)) {
                                // Search for the same component, by ID, in the previous journal.
                                EntityNetworkChanges::Page* prevComponentPage = nullptr;
                                int prevComponentPageIdx = prevPageIdx + 1;

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
                            int prevComponentPageIdx = prevPageIdx + 1;

                            while (auto prevComponentPage = previousJournal.findNextComponent(prevComponentPageIdx)) {
                                EntityNetworkChanges::Page* componentPage = nullptr;
                                int componentPageIdx = pageIdx + 1;

                                while ((componentPage = journal.findNextComponent(componentPageIdx)) != nullptr) {
                                    if (componentPage->componentId == prevComponentPage->componentId) {
                                        break;
                                    }
                                }

                                if (componentPage != nullptr) {
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

bool EntityNetworkSerialize::hasEntityChanges() const
{
    return hasComponentsAddedOrRemoved || !childrenAdded.empty() || !childrenRemoved.empty();
}

void EntityNetworkSerialize::getBytes(Bytes& data, const SerializerOptions& options) const
{
    data.resize_no_init(data.capacity());

    SerializerOptions opt(SerializerOptions::maxVersion);
    opt.dictionary = options.dictionary;

    Serializer s(data.byte_span(), opt);
    journal.writeJournal(s, scratchpad);

    data.resize(s.getSize());
}

void EntityNetworkSerialize::fetchNextPage(Deserializer& deserializer, EntityNetworkChanges::Type& type, uint16_t& size)
{
    type = EntityNetworkChanges::Type::Unknown;
    size = 0;

    if (deserializer.getBytesLeft() > 0) {
        uint8_t t;
        deserializer >> t;
        type = (EntityNetworkChanges::Type) t;
    }

    // Only fetch size for page types which actually contain data; keep size=0
    // for everything else.

    if (type == EntityNetworkChanges::Type::Entity ||
        type == EntityNetworkChanges::Type::Component) {

        if (deserializer.getBytesLeft() > 0) {
            uint16_t s;
            deserializer >> s;
            size = s;
        }
    }
}
