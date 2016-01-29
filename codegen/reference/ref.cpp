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
    IsometricCharacterSystem() : System({MainFamilyMask}) {
    }

  protected:
    // One of these:
    void Tick(Time elapsed); // Implement me
    void Tick(Time elapsed, EntityReference<MainFamilyType>& entity);

  private:

    struct MainFamilyType {
        IsometricCharacterComponent* const isometricCharacter;
        MovableComponent* const movable;
        const GravityComponent* const gravity;
    }

    FamilyBinding<MainFamilyType> mainFamily(MainFamilyMask);
}
