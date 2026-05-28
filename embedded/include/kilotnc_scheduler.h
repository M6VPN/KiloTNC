/* KiloTNC - Developed by M6VPN (M6VPN@tuta.com) */
/* KiloTNC/embedded/include/kilotnc_scheduler.h */

#ifndef KILOTNC_SCHEDULER_H
#define KILOTNC_SCHEDULER_H

#include <sys/types.h>

#include <stdint.h>

enum embedded_task_id {
	EMBEDDED_TASK_MAIN = 0,
	EMBEDDED_TASK_USB,
	EMBEDDED_TASK_AUDIO,
	EMBEDDED_TASK_MODEM_TX,
	EMBEDDED_TASK_MODEM_RX,
	EMBEDDED_TASK_CONTROL,
	EMBEDDED_TASK_DIAG,
	EMBEDDED_TASK_CONFIG,
	EMBEDDED_TASK_COUNT
};

enum embedded_scheduler_result {
	EMBEDDED_SCHEDULER_OK = 0,
	EMBEDDED_SCHEDULER_ERR_ARG,
	EMBEDDED_SCHEDULER_ERR_RANGE,
	EMBEDDED_SCHEDULER_ERR_FAULT
};

struct embedded_scheduler_status {
	uint32_t enabled_mask;
	uint32_t required_mask;
	uint32_t progress_mask;
	size_t cycles_completed;
	size_t fault_count;
	enum embedded_task_id last_failed_task;
	int faulted;
	int watchdog_allowed;
};

struct embedded_scheduler {
	struct embedded_scheduler_status status;
};

enum embedded_scheduler_result embedded_scheduler_cycle_complete(
	struct embedded_scheduler *);
enum embedded_scheduler_result embedded_scheduler_enable_task(
	struct embedded_scheduler *, enum embedded_task_id, int);
enum embedded_scheduler_result embedded_scheduler_fault(
	const struct embedded_scheduler *, int *);
enum embedded_scheduler_result embedded_scheduler_force_fault(
	struct embedded_scheduler *, enum embedded_task_id);
enum embedded_scheduler_result embedded_scheduler_init(
	struct embedded_scheduler *);
enum embedded_scheduler_result embedded_scheduler_mark_progress(
	struct embedded_scheduler *, enum embedded_task_id);
enum embedded_scheduler_result embedded_scheduler_require_task(
	struct embedded_scheduler *, enum embedded_task_id, int);
enum embedded_scheduler_result embedded_scheduler_status(
	const struct embedded_scheduler *, struct embedded_scheduler_status *);
enum embedded_scheduler_result embedded_scheduler_watchdog_allowed(
	const struct embedded_scheduler *, int *);

#endif
