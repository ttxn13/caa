#include "pch.hpp"
#include "ModuleMission.hpp"

QMutex ModuleMission::mutex;
ModuleMission* ModuleMission::instance = nullptr;

ModuleMission* ModuleMission::GetInstance()
{
	if (instance == nullptr) {
		mutex.lock();
		if (instance == nullptr) {
			instance = new ModuleMission();
		}
		mutex.unlock();
	}
	return instance;
}

void ModuleMission::DstInstance()
{
	delete instance;
	instance = nullptr;
}

ModuleMission::ModuleMission(QObject* parent)
	: QObject(parent)
{
	module_state = build_module_state(mission);
}

ModuleMission::~ModuleMission()
{
}

void ModuleMission::update_mission_source_text(const QString& source_text)
{
	update_mission(parse_mission_source_text(source_text));
}

void ModuleMission::update_mission(const S_MISSION& mission_state)
{
	QMutexLocker locker(&data_mutex);
	mission = mission_state;
	module_state = build_module_state(mission);
}

S_MISSION ModuleMission::mission_snapshot() const
{
	QMutexLocker locker(&data_mutex);
	return mission;
}

S_MODULE_STATE ModuleMission::module_state_snapshot() const
{
	QMutexLocker locker(&data_mutex);
	return module_state;
}

S_MISSION ModuleMission::parse_mission_source_text(const QString& source_text) const
{
	S_MISSION parsed_mission;
	parsed_mission.source_text = source_text;

	const QString trimmed_text = source_text.trimmed();
	if (trimmed_text.isEmpty()) {
		return parsed_mission;
	}

	const QRegularExpression mission_id_reg(
		QStringLiteral("\\bMIS[-_A-Za-z0-9]*\\b"),
		QRegularExpression::CaseInsensitiveOption);
	const QRegularExpressionMatch mission_id_match = mission_id_reg.match(trimmed_text);
	parsed_mission.mission_id = mission_id_match.hasMatch()
		? mission_id_match.captured(0)
		: QStringLiteral("MIS_MOVER-WORKFLOW-001");

	return parsed_mission;
}

S_MODULE_STATE ModuleMission::build_module_state(const S_MISSION& mission_state) const
{
	S_MODULE_STATE state;
	state.module_id = QStringLiteral("input-command");
	state.module_name = QStringLiteral("上级指令");
	state.update_time_ms = QDateTime::currentMSecsSinceEpoch();

	const bool has_source_text = !mission_state.source_text.trimmed().isEmpty();
	state.self_state = E_MODULE_RUN_STATE::DONE;
	state.effective_state = state.self_state;
	state.detail = has_source_text
		? (mission_state.mission_id.trimmed().isEmpty()
			? QStringLiteral("上级指令已解析")
			: QStringLiteral("上级指令已解析：%1").arg(mission_state.mission_id.trimmed()))
		: QStringLiteral("上级指令默认完成");

	return state;
}
