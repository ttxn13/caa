#pragma once
#include "pch.hpp"
#include "../../Core/Comm/CoreSystem.hpp"
#include "../../Core/Model/DefEngage.hpp"

class ModuleMission : public QObject
{
	Q_OBJECT

public:
	static ModuleMission* GetInstance();
	static void DstInstance();

public:
	void update_mission_source_text(const QString& source_text);
	void update_mission(const S_MISSION& mission_state);
	S_MISSION mission_snapshot() const;
	S_MODULE_STATE module_state_snapshot() const;

private:
	explicit ModuleMission(QObject* parent = nullptr);
	~ModuleMission() override;

	S_MISSION parse_mission_source_text(const QString& source_text) const;
	S_MODULE_STATE build_module_state(const S_MISSION& mission_state) const;

	mutable QMutex data_mutex;
	S_MISSION mission;
	S_MODULE_STATE module_state;

private:
	static QMutex mutex;
	static ModuleMission* instance;
};
