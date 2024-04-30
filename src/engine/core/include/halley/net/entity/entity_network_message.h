#pragma once

#include <cstdint>

#include "halley/data_structures/config_node.h"
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
    	SystemMsgResponse,
    	KeepAlive,
        JoinWorld,
        GetAccountData,
        SetAccountData
    };

    class IEntityNetworkMessage {
    public:
        virtual ~IEntityNetworkMessage() = default;
        virtual EntityNetworkHeaderType getType() const = 0;
        virtual void serialize(Serializer& s) const = 0;
    	virtual void deserialize(Deserializer& s) = 0;
        virtual bool needsWorld() const = 0;
    };

	class EntityNetworkMessageCreate final : public IEntityNetworkMessage {
	public:
        EntityNetworkId entityId;
        Bytes bytes;

        EntityNetworkMessageCreate() = default;
		EntityNetworkMessageCreate(EntityNetworkId id, Bytes bytes) : entityId(id), bytes(std::move(bytes)) {}

        EntityNetworkHeaderType getType() const override { return EntityNetworkHeaderType::Create; }
        bool needsWorld() const override { return true; }

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
        bool needsWorld() const override { return true; }

		void serialize(Serializer& s) const override;
        void deserialize(Deserializer& s) override;
	};

	class EntityNetworkMessageDestroy final : public IEntityNetworkMessage {
	public:
        EntityNetworkId entityId;

        EntityNetworkMessageDestroy() = default;
		EntityNetworkMessageDestroy(EntityNetworkId id) : entityId(id) {}

		EntityNetworkHeaderType getType() const override { return EntityNetworkHeaderType::Destroy; }
        bool needsWorld() const override { return true; }

		void serialize(Serializer& s) const override;
        void deserialize(Deserializer& s) override;
	};

	class EntityNetworkMessageReadyToStart final : public IEntityNetworkMessage {
	public:
        EntityNetworkHeaderType getType() const override { return EntityNetworkHeaderType::ReadyToStart; }
        bool needsWorld() const override { return false; }

		void serialize(Serializer& s) const override;
        void deserialize(Deserializer& s) override;
	};

	class EntityNetworkMessageEntityMsg final : public IEntityNetworkMessage {
	public:
        UUID entityUUID;
        int messageType = 0;
        Bytes messageData;

		EntityNetworkMessageEntityMsg() = default;
		EntityNetworkMessageEntityMsg(UUID entityUUID, int messageType, Bytes messageData) : entityUUID(entityUUID), messageType(messageType), messageData(std::move(messageData)) {}
		
		EntityNetworkHeaderType getType() const override { return EntityNetworkHeaderType::EntityMsg; }
        bool needsWorld() const override { return true; }

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
        bool needsWorld() const override { return true; }

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
        bool needsWorld() const override { return true; }

		void serialize(Serializer& s) const override;
        void deserialize(Deserializer& s) override;
	};

	class EntityNetworkMessageKeepAlive final : public IEntityNetworkMessage {
	public:
        EntityNetworkMessageKeepAlive() = default;

		EntityNetworkHeaderType getType() const override { return EntityNetworkHeaderType::KeepAlive; }
        bool needsWorld() const override { return false; }

		void serialize(Serializer& s) const override;
        void deserialize(Deserializer& s) override;
    };

    class EntityNetworkMessageJoinWorld final : public IEntityNetworkMessage {
    public:
        EntityNetworkMessageJoinWorld() = default;

        EntityNetworkHeaderType getType() const override { return EntityNetworkHeaderType::JoinWorld; }
        bool needsWorld() const override { return false; }

    	void serialize(Serializer& s) const override;
        void deserialize(Deserializer& s) override;
    };

    class EntityNetworkMessageGetAccountData final : public IEntityNetworkMessage {
    public:
        ConfigNode accountInfo;

        EntityNetworkMessageGetAccountData() = default;
        EntityNetworkMessageGetAccountData(ConfigNode info);

        EntityNetworkHeaderType getType() const override { return EntityNetworkHeaderType::GetAccountData; }
        bool needsWorld() const override { return false; }

    	void serialize(Serializer& s) const override;
        void deserialize(Deserializer& s) override;
    };

    class EntityNetworkMessageSetAccountData final : public IEntityNetworkMessage {
    public:
        ConfigNode accountData;

        EntityNetworkMessageSetAccountData() = default;
        EntityNetworkMessageSetAccountData(ConfigNode data);

        EntityNetworkHeaderType getType() const override { return EntityNetworkHeaderType::SetAccountData; }
        bool needsWorld() const override { return false; }

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

        bool needsWorld() const;

        void serialize(Serializer& s) const;
        void deserialize(Deserializer& s);

    private:
        std::unique_ptr<IEntityNetworkMessage> message;
    };
}
