template <int numComponents>
struct EntityRawReference {
    EntityId id;
    std::array<void*, numComponents> components;
}

template <typename T>
struct EntityReference {
    EntityID id;
    T components;
}

class IsometricCharacterSystem : public System {
  public:
    IsometricCharacterSystem() : System({MainFamily::mask}) {
    }

  protected:
    // One of these:
    void Tick(Time elapsed) override; // Implement me
    void Tick(Time elapsed, EntityReference<MainFamily>& entity) override; // Implement me

  private:

    struct MainFamily {
        constexpr FamilyMask mask = /*...*/;

        IsometricCharacterComponent* const isometricCharacter;
        MovableComponent* const movable;
        const GravityComponent* const gravity;
    };

    FamilyBinding<MainFamilyType> mainFamily;
}
