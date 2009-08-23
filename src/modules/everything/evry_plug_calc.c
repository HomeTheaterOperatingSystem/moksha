#include "Evry.h"


static int  _cb_data(void *data, int type, void *event);
static int  _cb_error(void *data, int type, void *event);
static int  _cb_del(void *data, int type, void *event);

static Evry_Plugin *p1;
static Ecore_Exe *exe = NULL;
static Eina_List *history = NULL;
static Eina_List *handlers = NULL;

static int error = 0;



static Evry_Plugin *
_begin(Evry_Plugin *p, const Evry_Item *item __UNUSED__)
{
   Evry_Item *it;

   if (history)
     {
	const char *result;

	EINA_LIST_FREE(history, result)
	  {
	     it = evry_item_new(NULL, p, result, NULL);
	     p->items = eina_list_prepend(p->items, it);
	     eina_stringshare_del(result);
	  }
     }

   it = evry_item_new(NULL, p, "0", NULL);
   p->items = eina_list_prepend(p->items, it);

   return p;
}

static int
_run_bc(Evry_Plugin *p)
{
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EXE_EVENT_DATA, _cb_data, p));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EXE_EVENT_ERROR, _cb_error, p));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EXE_EVENT_DEL, _cb_del, p));

   exe = ecore_exe_pipe_run("bc -l",
			    ECORE_EXE_PIPE_READ |
			    ECORE_EXE_PIPE_READ_LINE_BUFFERED |
			    ECORE_EXE_PIPE_WRITE |
			    ECORE_EXE_PIPE_ERROR |
			    ECORE_EXE_PIPE_ERROR_LINE_BUFFERED,
			    NULL);
   return !!exe;
}


static void
_cleanup(Evry_Plugin *p)
{
   Ecore_Event_Handler *h;
   Evry_Item *it;
   int items = 10;

   if (p->items)
     {
	evry_item_free(p->items->data);
	p->items = eina_list_remove_list(p->items, p->items);
     }

   EINA_LIST_FREE(p->items, it)
     {
	if (items-- > 0)
	  history = eina_list_prepend(history, eina_stringshare_add(it->label));

	evry_item_free(it);
     }

   EINA_LIST_FREE(handlers, h)
     ecore_event_handler_del(h);

   if (exe)
     {
	ecore_exe_quit(exe);
	ecore_exe_free(exe);
	exe = NULL;
     }
}

static int
_action(Evry_Plugin *p, const Evry_Item *it)
{
   Eina_List *l;
   Evry_Item *it2;

   /* remove duplicates */
   if (p->items->next)
     {
	it = p->items->data;

	EINA_LIST_FOREACH(p->items->next, l, it2)
	  {
	     if (!strcmp(it->label, it2->label))
	       break;
	     it2 = NULL;
	  }

	if (it2)
	  {
	     p->items = eina_list_remove(p->items, it2);
	     evry_item_free(it2);
	  }
     }

   it = p->items->data;

   it2 = evry_item_new(NULL, p, it->label, NULL);
   p->items = eina_list_prepend(p->items, it2);

   evry_plugin_async_update(p, EVRY_ASYNC_UPDATE_ADD);

   return EVRY_ACTION_FINISHED;
}

static int
_fetch(Evry_Plugin *p, const char *input)
{
   char buf[1024];

   if (!input) return 0;

   if (!exe && !_run_bc(p)) return 0;

   if (!strncmp(input, "scale=", 6))
     snprintf(buf, 1024, "%s\n", input);
   else
     snprintf(buf, 1024, "scale=3;%s\n", input);

   ecore_exe_send(exe, buf, strlen(buf));

   /* XXX after error we get no response for first input ?! - send a
      second time...*/
   if (error)
     {
	ecore_exe_send(exe, buf, strlen(buf));
	error = 0;
     }

   return 1;
}

static int
_cb_data(void *data, int type __UNUSED__, void *event)
{
   Ecore_Exe_Event_Data *ev = event;
   Evry_Plugin *p = data;
   Evry_Item *it;

   if (ev->exe != exe) return 1;

   if (ev->lines)
     {
	it = p->items->data;
	p->items = eina_list_remove(p->items, it);
	evry_item_free(it);

	it = evry_item_new(NULL, p, ev->lines->line, NULL);
	p->items = eina_list_prepend(p->items, it);
     }

   evry_plugin_async_update(p, EVRY_ASYNC_UPDATE_ADD);

   return 1;
}

static int
_cb_error(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Exe_Event_Data *ev = event;

   if (ev->exe != exe)
     return 1;

   error = 1;

   return 1;
}

static int
_cb_del(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Exe_Event_Del *e = event;

   if (e->exe != exe)
     return 1;

   exe = NULL;
   return 1;
}

static Eina_Bool
_init(void)
{
   p1 = evry_plugin_new(NULL, "Calculator", type_subject, NULL, "TEXT", 1, "accessories-calculator", "=",
			_begin, _cleanup, _fetch, _action, NULL, NULL, NULL);

   evry_plugin_register(p1, 0);

   return EINA_TRUE;
}

static void
_shutdown(void)
{
   char *result;

   EINA_LIST_FREE(history, result)
     eina_stringshare_del(result);

   EVRY_PLUGIN_FREE(p1);
}


EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);
