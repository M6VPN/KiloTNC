/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/app/embedded_scheduler.c */

#include <sys/types.h>

#include <stdint.h>
#include <string.h>

#include "embedded_scheduler.h"

static enum embedded_task_id embedded_scheduler_first_missing(uint32_t);
static int embedded_scheduler_task_valid(enum embedded_task_id);
static uint32_t embedded_scheduler_task_mask(enum embedded_task_id);

static enum embedded_task_id
embedded_scheduler_first_missing(uint32_t mask)
{
	enum embedded_task_id task;

	for (task = EMBEDDED_TASK_MAIN; task < EMBEDDED_TASK_COUNT;
	    task = (enum embedded_task_id)(task + 1)) {
		if ((mask & embedded_scheduler_task_mask(task)) != 0U)
			return task;
	}

	return EMBEDDED_TASK_COUNT;
}

static int
embedded_scheduler_task_valid(enum embedded_task_id task)
{
	if (task < EMBEDDED_TASK_MAIN)
		return 0;
	if (task >= EMBEDDED_TASK_COUNT)
		return 0;

	return 1;
}

static uint32_t
embedded_scheduler_task_mask(enum embedded_task_id task)
{
	return 1U << (uint32_t)task;
}

enum embedded_scheduler_result
embedded_scheduler_cycle_complete(struct embedded_scheduler *scheduler)
{
	uint32_t missing;

	if (scheduler == NULL)
		return EMBEDDED_SCHEDULER_ERR_ARG;
	if (scheduler->status.faulted != 0) {
		scheduler->status.watchdog_allowed = 0;
		return EMBEDDED_SCHEDULER_ERR_FAULT;
	}

	missing = scheduler->status.required_mask &
	    ~scheduler->status.progress_mask;
	if (missing != 0U) {
		scheduler->status.faulted = 1;
		scheduler->status.fault_count++;
		scheduler->status.last_failed_task =
		    embedded_scheduler_first_missing(missing);
		scheduler->status.watchdog_allowed = 0;
		return EMBEDDED_SCHEDULER_ERR_FAULT;
	}

	scheduler->status.cycles_completed++;
	scheduler->status.watchdog_allowed = 1;
	scheduler->status.progress_mask = 0U;
	return EMBEDDED_SCHEDULER_OK;
}

enum embedded_scheduler_result
embedded_scheduler_enable_task(struct embedded_scheduler *scheduler,
	enum embedded_task_id task, int enabled)
{
	uint32_t mask;

	if (scheduler == NULL)
		return EMBEDDED_SCHEDULER_ERR_ARG;
	if (!embedded_scheduler_task_valid(task))
		return EMBEDDED_SCHEDULER_ERR_RANGE;

	mask = embedded_scheduler_task_mask(task);
	if (enabled != 0) {
		scheduler->status.enabled_mask |= mask;
		return EMBEDDED_SCHEDULER_OK;
	}

	scheduler->status.enabled_mask &= ~mask;
	scheduler->status.required_mask &= ~mask;
	scheduler->status.progress_mask &= ~mask;
	return EMBEDDED_SCHEDULER_OK;
}

enum embedded_scheduler_result
embedded_scheduler_fault(const struct embedded_scheduler *scheduler,
	int *faulted)
{
	if (scheduler == NULL || faulted == NULL)
		return EMBEDDED_SCHEDULER_ERR_ARG;

	*faulted = scheduler->status.faulted;
	return EMBEDDED_SCHEDULER_OK;
}

enum embedded_scheduler_result
embedded_scheduler_force_fault(struct embedded_scheduler *scheduler,
	enum embedded_task_id task)
{
	if (scheduler == NULL)
		return EMBEDDED_SCHEDULER_ERR_ARG;
	if (!embedded_scheduler_task_valid(task))
		return EMBEDDED_SCHEDULER_ERR_RANGE;

	if (scheduler->status.faulted == 0) {
		scheduler->status.fault_count++;
		scheduler->status.last_failed_task = task;
	}
	scheduler->status.faulted = 1;
	scheduler->status.watchdog_allowed = 0;
	return EMBEDDED_SCHEDULER_OK;
}

enum embedded_scheduler_result
embedded_scheduler_init(struct embedded_scheduler *scheduler)
{
	uint32_t main_mask;
	uint32_t control_mask;

	if (scheduler == NULL)
		return EMBEDDED_SCHEDULER_ERR_ARG;

	(void)memset(scheduler, 0, sizeof(*scheduler));
	main_mask = embedded_scheduler_task_mask(EMBEDDED_TASK_MAIN);
	control_mask = embedded_scheduler_task_mask(EMBEDDED_TASK_CONTROL);
	scheduler->status.enabled_mask = main_mask | control_mask;
	scheduler->status.required_mask = main_mask | control_mask;
	scheduler->status.last_failed_task = EMBEDDED_TASK_COUNT;
	return EMBEDDED_SCHEDULER_OK;
}

enum embedded_scheduler_result
embedded_scheduler_mark_progress(struct embedded_scheduler *scheduler,
	enum embedded_task_id task)
{
	uint32_t mask;

	if (scheduler == NULL)
		return EMBEDDED_SCHEDULER_ERR_ARG;
	if (!embedded_scheduler_task_valid(task))
		return EMBEDDED_SCHEDULER_ERR_RANGE;
	if (scheduler->status.faulted != 0)
		return EMBEDDED_SCHEDULER_ERR_FAULT;

	mask = embedded_scheduler_task_mask(task);
	if ((scheduler->status.enabled_mask & mask) == 0U)
		return EMBEDDED_SCHEDULER_ERR_RANGE;

	scheduler->status.progress_mask |= mask;
	return EMBEDDED_SCHEDULER_OK;
}

enum embedded_scheduler_result
embedded_scheduler_require_task(struct embedded_scheduler *scheduler,
	enum embedded_task_id task, int required)
{
	uint32_t mask;

	if (scheduler == NULL)
		return EMBEDDED_SCHEDULER_ERR_ARG;
	if (!embedded_scheduler_task_valid(task))
		return EMBEDDED_SCHEDULER_ERR_RANGE;

	mask = embedded_scheduler_task_mask(task);
	if (required != 0) {
		scheduler->status.enabled_mask |= mask;
		scheduler->status.required_mask |= mask;
		return EMBEDDED_SCHEDULER_OK;
	}

	scheduler->status.required_mask &= ~mask;
	return EMBEDDED_SCHEDULER_OK;
}

enum embedded_scheduler_result
embedded_scheduler_status(const struct embedded_scheduler *scheduler,
	struct embedded_scheduler_status *status)
{
	if (scheduler == NULL || status == NULL)
		return EMBEDDED_SCHEDULER_ERR_ARG;

	*status = scheduler->status;
	return EMBEDDED_SCHEDULER_OK;
}

enum embedded_scheduler_result
embedded_scheduler_watchdog_allowed(
	const struct embedded_scheduler *scheduler, int *allowed)
{
	if (scheduler == NULL || allowed == NULL)
		return EMBEDDED_SCHEDULER_ERR_ARG;

	*allowed = scheduler->status.watchdog_allowed;
	return EMBEDDED_SCHEDULER_OK;
}
