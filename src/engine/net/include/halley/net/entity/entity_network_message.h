#pragma once

#include <cstdint>

#include "halley/entity/system_message.h"
#include "halley/maths/uuid.h"

namespace Halley {
    using EntityNetworkId = uint16_t;

    enum class EntityNetworkHeaderType : uint8_t {
        Create,
        Update,
        Destroy,
    	ReadyToStart,
    	EntityMsg,
    	SystemMsg,
    	SystemMsgResponse
    };

    class IEntityNetworkMessage {
    public:
        virtual ~IEntityNetworkMessage() = default;
        virtual EntityNetworkHeaderType getType() const = 0;
        virtual void serialize(Serializer& s) const = 0;
    	virtual void deserialize(Deserializer& s) = 0;
        virtual bool needsInitialization() const { return true; }
    };

	class EntityNetworkMessageCreate final : public IEntityNetworkMessage {
	public:
        EntityNetworkId entityId;
        Bytes bytes;

        EntityNetworkMessageCreate() = default;
		EntityNetworkMessageCreate(EntityNetworkId id, Bytes bytes) : entityId(id), bytes(std::move(bytes)) {}

        EntityNetworkHeaderType getType() const override { return EntityNetworkHeaderType::Create; }
        void serialize(Serializer& s) const override;
        void deserialize(Deserializer& s) override;
	};

	class EntityNetworkMessageUpdate final : public IEntityNetworkMessage {
	public:
        EntityNetworkId entityId;
        Bytes bytes;

        EntityNetworkMessageUpdate() = default;
		EntityNetworkMessageUpdate(EntityNetworkId id, Bytes bytes) : entityId(id), bytes(std::move(bytes)) {}

		EntityNetworkHeaderType getType() const override { return EntityNetworkHeaderType::Update; }
		void serialize(Serializer& s) const override;
        void deserialize(Deserializer& s) override;
	};

	class EntityNetworkMessageDestroy final : public IEntityNetworkMessage {
	public:
        EntityNetworkId entityId;

        EntityNetworkMessageDestroy() = default;
		EntityNetworkMessageDestroy(EntityNetworkId id) : entityId(id) {}

		EntityNetworkHeaderType getType() const override { return EntityNetworkHeaderType::Destroy; }
		void serialize(Serializer& s) const override;
        void deserialize(Deserializer& s) override;
	};

	class EntityNetworkMessageReadyToStart final : public IEntityNetworkMessage {
	public:
        EntityNetworkHeaderType getType() const override { return EntityNetworkHeaderType::ReadyToStart; }
		void serialize(Serializer& s) const override;
        void deserialize(Deserializer& s) override;
        bool needsInitialization() const override { return false; }
	};

	class EntityNetworkMessageEntityMsg final : public IEntityNetworkMessage {
	public:
        UUID entityUUID;
        int messageType = 0;
        Bytes messageData;

		EntityNetworkMessageEntityMsg() = default;
		EntityNetworkMessageEntityMsg(UUID entityUUID, int messageType, Bytes messageData) : entityUUID(entityUUID), messageType(messageType), messageData(std::move(messageData)) {}
		
		EntityNetworkHeaderType getType() const override { return EntityNetworkHeaderType::EntityMsg; }
		void serialize(Serializer& s) const override;
        void deserialize(Deserializer& s) override;
	};

	class EntityNetworkMessageSystemMsg final : public IEntityNetworkMessage {
	public:
        int messageType = 0;
        uint32_t msgId = 0;
        bool wantsResponse = false;
        String targetSystem;
        SystemMessageDestination destination;
        Bytes messageData;

		EntityNetworkMessageSystemMsg() = default;
		EntityNetworkMessageSystemMsg(int messageType, uint32_t msgId, bool wantsResponse, String targetSystem, SystemMessageDestination destination, Bytes messageData)
			: messageType(messageType)
			, msgId(msgId)
			, wantsResponse(wantsResponse)
			, targetSystem(std::move(targetSystem))
			, destination(destination)
			, messageData(std::move(messageData))
		{}
		
		EntityNetworkHeaderType getType() const override { return EntityNetworkHeaderType::SystemMsg; }
		void serialize(Serializer& s) const override;
        void deserialize(Deserializer& s) override;
	};

	class EntityNetworkMessageSystemMsgResponse final : public IEntityNetworkMessage {
	public:
        int messageType = 0;
        uint32_t msgId = 0;
        Bytes responseData;

		EntityNetworkMessageSystemMsgResponse() = default;
		EntityNetworkMessageSystemMsgResponse(int messageType, uint32_t msgId, Bytes responseData) : messageType(messageType), msgId(msgId), responseData(std::move(responseData)) {}
		
		EntityNetworkHeaderType getType() const override { return EntityNetworkHeaderType::SystemMsgResponse; }
		void serialize(Serializer& s) const override;
        void deserialize(Deserializer& s) override;
	};
    
    class EntityNetworkMessage {
    public:
        EntityNetworkMessage() = default;
        explicit EntityNetworkMessage(std::unique_ptr<IEntityNetworkMessage> msg);

    	template <typename T>
        EntityNetworkMessage(T msg)
    	{
            static_assert(std::is_base_of_v<IEntityNetworkMessage, T>);
            setMessage(std::make_unique<T>(std::move(msg)));
    	}

        void setMessage(std::unique_ptr<IEntityNetworkMessage> msg);
        EntityNetworkHeaderType getType() const { return message->getType(); }

    	template <typename T>
        T& getMessage() const
        {
            return dynamic_cast<T&>(*message);
        }

        bool needsInitialization() const;

        void serialize(Serializer& s) const;
        void deserialize(Deserializer& s);

    private:
        std::unique_ptr<IEntityNetworkMessage> message;
    };
}
