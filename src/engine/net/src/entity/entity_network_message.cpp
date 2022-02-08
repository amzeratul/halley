#include "entity/entity_network_message.h"

#include <cassert>

#include "halley/bytes/byte_serializer.h"

using namespace Halley;

void EntityNetworkMessageCreate::serialize(Serializer& s) const
{
	s << entityId;
	s << bytes;
}

void EntityNetworkMessageCreate::deserialize(Deserializer& s)
{
	s >> entityId;
	s >> bytes;
}

void EntityNetworkMessageUpdate::serialize(Serializer& s) const
{
	s << entityId;
	s << bytes;
}

void EntityNetworkMessageUpdate::deserialize(Deserializer& s)
{
	s >> entityId;
	s >> bytes;
}

void EntityNetworkMessageDestroy::serialize(Serializer& s) const
{
	s << entityId;
}

void EntityNetworkMessageDestroy::deserialize(Deserializer& s)
{
	s >> entityId;
}

void EntityNetworkMessageReadyToStart::serialize(Serializer& s) const
{}

void EntityNetworkMessageReadyToStart::deserialize(Deserializer& s)
{}

void EntityNetworkMessageMessageToEntity::serialize(Serializer& s) const
{
	s << messageType;
	s << messageData;
}

void EntityNetworkMessageMessageToEntity::deserialize(Deserializer& s)
{
	s >> messageType;
	s >> messageData;
}

EntityNetworkMessage::EntityNetworkMessage(std::unique_ptr<IEntityNetworkMessage> msg)
	: message(std::move(msg))
{
}

void EntityNetworkMessage::setMessage(std::unique_ptr<IEntityNetworkMessage> msg)
{
	message = std::move(msg);
}

bool EntityNetworkMessage::needsInitialization() const
{
	return message->needsInitialization();
}

void EntityNetworkMessage::serialize(Serializer& s) const
{
	s << static_cast<int>(getType());
	message->serialize(s);
}

void EntityNetworkMessage::deserialize(Deserializer& s)
{
	int t;
	s >> t;
	const auto type = static_cast<EntityNetworkHeaderType>(t);

	switch (type) {
	case EntityNetworkHeaderType::Create:
		message = std::make_unique<EntityNetworkMessageCreate>();
		break;
	case EntityNetworkHeaderType::Update:
		message = std::make_unique<EntityNetworkMessageUpdate>();
		break;
	case EntityNetworkHeaderType::Destroy:
		message = std::make_unique<EntityNetworkMessageDestroy>();
		break;
	case EntityNetworkHeaderType::ReadyToStart:
		message = std::make_unique<EntityNetworkMessageReadyToStart>();
		break;
	case EntityNetworkHeaderType::MessageToEntity:
		message = std::make_unique<EntityNetworkMessageMessageToEntity>();
		break;
	}

	assert(message && message->getType() == type);

	message->deserialize(s);
}
