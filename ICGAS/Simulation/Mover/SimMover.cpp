#include "pch.hpp"
#include "SimMover.hpp"
#include "SimAirMover.hpp"
#include "SimSeaMover.hpp"
#include "SimMisMover.hpp"

std::unique_ptr<SimMover> make_sim_mover(E_MOVER type)
{
	switch (type) {
	case E_MOVER::AIR_MOVER:	return std::make_unique<SimAirMover>();
	case E_MOVER::SEA_MOVER:	return std::make_unique<SimSeaMover>();
	case E_MOVER::MIS_MOVER:	return std::make_unique<SimMisMover>();
	default:					return nullptr;
	}
}
