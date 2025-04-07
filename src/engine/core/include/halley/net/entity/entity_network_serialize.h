#pragma once

#include "halley/entity/entity.h"
#include "halley/entity/entity_factory.h"
#include "halley/entity/world.h"
#include "halley/utils/hash.h"
#include "halley/entity/data_interpolator.h"
#include "halley/entity/byte_data_interpolator.h"
#include "components/network_component.h"

namespace Halley {

    class IByteDataInterpolatorSet;
    class SerializerOptions;

    class EntityNetworkChanges
    {
    public:
        enum class Type : uint8_t
        {
            Unknown = 0,
            Entity,
            EntityIdentity,
            Component,
        };

        struct Page
        {
            explicit Page() {};

            union
            {
                UUID uuid;
                uint16_t componentId = 0;
            };

            uint64_t hash = 0; // XXH64 over data in scratchpad

            uint16_t from = 0; // points to start of data in scratchpad
            uint16_t to = 0; // points to end of data in scratchpad

            Type type = Type::Unknown;

            // Volatile, intermediate, per-frame data; not serialized!

            mutable bool modified = false;
        };

        explicit EntityNetworkChanges() = default;

        void pushEntity(Serializer& serializer, const EntityRef& entity, std::optional<EntityRef> parent, Bytes& buffer);

        void beginComponent(Serializer& serializer, uint16_t componentId);
        void endComponent(Serializer& serializer, Bytes& buffer);

        void digest();
        bool isFull() const;

        void serialize(Serializer& s) const;
        void deserialize(Deserializer& s);

        [[nodiscard]] size_t getRequiredSerializeSize() const;

        bool operator==(const EntityNetworkChanges& other) const;

        void writeJournal(Serializer& serializer, const Bytes& buffer, bool log) const;

        void enumerateEntityPages(const HashSet<UUID>& filter,
                const std::function<void (const Page& page, int pageIdx)>& onEntity) const;

        void enumerateEntities(const std::function<void (const Page& page, int pageIdx)>& onEntity) const;

        Page* getEntityIdentity(int pageIdx);
        Page* findNextComponent(int& pageIdx);

        const Page& findEntityByUUID(const UUID& uuid, int& pageIdx) const;

    private:
        Page curPage;

        std::array<Page, 512> pages;
        int pp = 0;

        uint64_t contentHash = 0;
        Hash::Hasher contentHasher;

        void beginPage(Serializer& serializer, Type type);
        void endPage(Serializer& serializer, Bytes& buffer, Type type);
    };

    class EntityNetworkSerialize
    {
    public:
        explicit EntityNetworkSerialize(Resources& resources, const IByteDataInterpolatorSet* interpolators);

        bool serializeEntityUpdate(const EntityRef& entity, const SerializerOptions& options);
        void deserializeEntityUpdate(EntityRef& entity, const std::shared_ptr<const Prefab>& prefab, const Bytes& bytes, const SerializerOptions& options);

        bool processEntityUpdateChanges(Bytes& previous);
        bool hasEntityChanges() const;

        void getBytes(Bytes& data, const SerializerOptions& options, bool log) const;

    private:
        class SerializationContext : public IEntityFactoryContext
        {
        public:
            explicit SerializationContext(const EntityRef& root, const std::shared_ptr<const Prefab>& prefab = {})
                : entity({})
                , prefab(prefab)
            {
                if (const auto networkComponent = root.tryGetComponent<NetworkComponent>()) {
                    interpolators = &networkComponent->byteDataInterpolatorSet;
                }
            }

            EntityId getEntityIdFromUUID(const UUID &uuid) const override
            {
                if (const auto e = entity.getWorld().findEntity(uuid)) {
                    return e->getEntityId();
                }
                return {};
            }

            UUID getUUIDFromEntityId(EntityId id) const override
            {
                if (const auto e = entity.getWorld().tryGetEntity(id); e.isValid()) {
                    return e.getInstanceUUID();
                }
                return {};
            }

            EntityId getCurrentEntityId() const override
            {
                return entity.getEntityId();
            }

            void setCurrentEntity(const EntityRef& entity) const
            {
                this->entity = entity;
            }

            const std::shared_ptr<const Prefab>& getPrefab() const
            {
                return prefab;
            }

            const IByteDataInterpolatorSet* getByteDataInterpolators() const
            {
                return interpolators;
            }

        private:
            mutable EntityRef entity;
            const std::shared_ptr<const Prefab> prefab;
            const IByteDataInterpolatorSet* interpolators = nullptr;
        };

        void doSerializeEntityUpdate(
            const SerializationContext& context, Serializer& serializer,
            const EntityRef& entity, const std::optional<EntityRef>& parent);

        EntityNetworkChanges::Type doDeserializeEntityUpdate(
            const SerializationContext& context, Deserializer& deserializer,
            EntityRef& entity, const std::optional<EntityRef>& parent);

        static void fetchNextPage(Deserializer& deserializer, EntityNetworkChanges::Type& type, uint16_t& size);
        static std::optional<EntityRef> findChildEntity(const EntityRef& entity, const UUID& instanceUUID);

        Resources& resources;
        const IByteDataInterpolatorSet* interpolators;
        EntityNetworkChanges journal;

        bool hasComponentsAddedOrRemoved;

        static thread_local Bytes scratchpad;
    };

}
