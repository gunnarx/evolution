install(FILES e-calendar-preferences.ui
	DESTINATION ${uidir}
)

set(extra_deps
	econtacteditor
	econtactlisteditor
	evolution-calendar
	evolution-calendar-importers
	evolution-mail
	evolution-shell
	gnomecanvas
)
set(sources
	evolution-module-calendar.c
	e-cal-attachment-handler.c
	e-cal-attachment-handler.h
	e-cal-base-shell-backend.c
	e-cal-base-shell-backend.h
	e-cal-base-shell-content.c
	e-cal-base-shell-content.h
	e-cal-base-shell-sidebar.c
	e-cal-base-shell-sidebar.h
	e-cal-base-shell-view.c
	e-cal-base-shell-view.h
	e-cal-config-hook.c
	e-cal-config-hook.h
	e-cal-event-hook.c
	e-cal-event-hook.h
	e-cal-shell-backend.c
	e-cal-shell-backend.h
	e-cal-shell-content.c
	e-cal-shell-content.h
	e-cal-shell-migrate.c
	e-cal-shell-migrate.h
	e-cal-shell-view.c
	e-cal-shell-view.h
	e-cal-shell-view-actions.c
	e-cal-shell-view-actions.h
	e-cal-shell-view-private.c
	e-cal-shell-view-private.h
	e-cal-shell-view-memopad.c
	e-cal-shell-view-taskpad.c
	e-calendar-preferences.c
	e-calendar-preferences.h
	e-memo-shell-backend.c
	e-memo-shell-backend.h
	e-memo-shell-content.c
	e-memo-shell-content.h
	e-memo-shell-migrate.c
	e-memo-shell-migrate.h
	e-memo-shell-view.c
	e-memo-shell-view.h
	e-memo-shell-view-actions.c
	e-memo-shell-view-actions.h
	e-memo-shell-view-private.c
	e-memo-shell-view-private.h
	e-task-shell-backend.c
	e-task-shell-backend.h
	e-task-shell-content.c
	e-task-shell-content.h
	e-task-shell-migrate.c
	e-task-shell-migrate.h
	e-task-shell-view.c
	e-task-shell-view.h
	e-task-shell-view-actions.c
	e-task-shell-view-actions.h
	e-task-shell-view-private.c
	e-task-shell-view-private.h
)
set(extra_defines
	-DEVOLUTION_PRIVLIBEXECDIR=\"${privlibexecdir}\"
)
set(extra_cflags)
set(extra_incdirs)
set(extra_ldflags
	${MATH_LDFLAGS}
)

add_evolution_module(module-calendar
	sources
	extra_deps
	extra_defines
	extra_cflags
	extra_incdirs
	extra_ldflags
)
