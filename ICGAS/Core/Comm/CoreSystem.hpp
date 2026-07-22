#pragma once
#include "pch.hpp"

enum class E_WORKFLOW_RUN_STATE {
	STOPPED,
	RUNNING,
	PAUSED,
};

enum class E_MODULE_RUN_STATE {
	UNKNOWN,
	NOT_READY,
	READY,
	WAITING,
	RUNNING,
	DONE,
	FAILED,
	DISABLED,
};

enum class E_MODULE_DEPENDENCY_MODE {
	ALL_READY,
	ANY_READY,
};

struct S_MODULE_STATE {
	QString module_id;
	QString module_name;
	E_MODULE_RUN_STATE self_state		= E_MODULE_RUN_STATE::READY;
	E_MODULE_RUN_STATE parent_state		= E_MODULE_RUN_STATE::READY;
	E_MODULE_RUN_STATE effective_state	= E_MODULE_RUN_STATE::READY;
	E_MODULE_DEPENDENCY_MODE dependency_mode = E_MODULE_DEPENDENCY_MODE::ALL_READY;
	std::vector<QString> prerequisite_module_ids;
	std::vector<std::vector<QString>> prerequisite_any_module_id_groups;
	QString detail;
	QString error_text;
	qint64 update_time_ms = 0;
	bool required = true;
};
