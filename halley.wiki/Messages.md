# Overview
Messages can be sent from any [[system|Systems]] to any [[entity|Entities]]. Any other system can then capture that message, provided that:

1. It has the recipient entity in its main family
1. It has registered itself to receive that type of message

Typically, you can have multiple systems sending the same type of message, but you usually only want one system receiving each type of message.

Messages allow for an efficient way for systems to communicate with each other about events happening regarding entities, such as collisions.

# Declaration
Messages have the same syntax as [[Components]], see their documentation for more information. The only difference is that they are labelled `message` instead. For example:

```yaml
message:
  name: CameraShake
  members:
    - offset: 'Halley::Vector2f'
    - duration: float
```