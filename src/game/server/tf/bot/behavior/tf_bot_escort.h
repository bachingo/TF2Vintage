/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/tf_bot_escort.h
 * used in MvM: TODO
 */


// sizeof: TODO (>=0x4824)
class CTFBotEscort : public Action<CTFBot>
{
public:
	CTFBotEscort(CBaseEntity *who);
	virtual ~CTFBotEscort();
	
	virtual const char *GetName() const OVERRIDE;
	
	virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) OVERRIDE;
	virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) OVERRIDE;
	
	virtual EventDesiredResult<CTFBot> OnMoveToSuccess(CTFBot *actor, const Path *path) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnMoveToFailure(CTFBot *actor, const Path *path, MoveToFailureType fail) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnStuck(CTFBot *actor) OVERRIDE;
	virtual EventDesiredResult<CTFBot> OnCommandApproach(CTFBot *actor, const Vector& v1, float f1) OVERRIDE;
	
	virtual QueryResponse ShouldRetreat(const INextBot *nextbot) const OVERRIDE;
	
	CBaseEntity *GetWho() const;
	void SetWho(CBaseEntity *who);
	
private:
	CBaseEntity *m_hWho;              // +0x0034
	PathFollower m_PathFollower;      // +0x0038
	// 480c CountdownTimer
	CountdownTimer m_ctRecomputePath; // +0x4818
};
