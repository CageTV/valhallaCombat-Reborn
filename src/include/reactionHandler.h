/*Handle all hit reactions: triggerStagger, knock back, guard break etc*/
class reactionHandler {
public:
	enum reactionType : std::uint32_t
	{
		kNone = 0,
		kSmall = 1,
		kMedium = 2,
		kLarge = 3,
		kLargest = 4,
	};
	
	/*Trigger a stagger reaction.
	@param causer: actor that causes the stagger.
	@param reactor: actor receiving the stagger.
	@param reactionType: type of the reaction the actor receives.*/
	static void applyStagger(RE::Actor* causer, RE::Actor* reactor, reactionType reactionType);

	/*Trigger a recoil reaction.*/
	static void triggerRecoil(RE::Actor* reactor);
	/*Initialize a downed state for an actor.
	@param a_actor: actor to enter downed state*/
	static void triggerDownedState(RE::Actor* a_actor);

	/*Recover an actor from the downed state.
	@param a_actor: actor to recover from downed state.*/
	static void recoverDownedState(RE::Actor* a_actor);
private:

	/*Stagger the reactor with magnitude and correct stagger direction.
	@param causer: the cause of the stagger.
	@param reactor: actor who staggers.
	@param magnitude: magnitude of the triggerStagger.*/
	static void triggerStagger(RE::Actor* causer, RE::Actor* reactor, float magnitude);

	/*Inflict a "poise" reaction onto reactor.
	@param causer: the cause of the reaction.
	@param reactor: actor who react.
	@param reactionType: type of the poise reaction.*/
	static void triggerPoiseReaction(RE::Actor* causer, RE::Actor* reactor, reactionType reactionType);

	
};
