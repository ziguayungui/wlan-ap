/* SPDX-License-Identifier: BSD-3-Clause */

#include <ev.h>
#include <evsched.h>
#include <target.h>

#include <libubox/avl-cmp.h>
#include <libubox/avl.h>

#include "ubus.h"

struct ubus_remote {
	struct avl_node avl;
        char name[64];
	uint32_t id;
};

struct ev_loop *loop;
static ev_io ubus_io;

static struct ubus_context ubus;
//static struct ubus_context ctx;
static struct ubus_subscriber ubus_subscriber;
static struct ubus_instance *ubus_instance;

static struct avl_tree ubus_tree = AVL_TREE_INIT(ubus_tree, avl_strcmp, false, NULL);

uint32_t ubus_lookup_remote(char *name)
{
	struct ubus_remote *remote;

	remote = avl_find_element(&ubus_tree, name, remote, avl);
	if (!remote)
		return 0;

	return remote->id;
}

static void ubus_handle_subscribe(const char *path, uint32_t id, int add)
{
	struct ubus_remote *remote;
	const char *name;
	int i;

	for (i = 0; i < ubus_instance->len; i++) {
		int len = strlen(ubus_instance->list[i].path);

		if (ubus_instance->list[i].wildcard && !strncmp(path, ubus_instance->list[i].path, len))
			break;
		if (!ubus_instance->list[i].wildcard && !strcmp(path, ubus_instance->list[i].path))
			break;
	}

	if (i >= ubus_instance->len)
		return;

	name = path;
	if (ubus_instance->list[i].wildcard)
		name = &path[strlen(ubus_instance->list[i].path)];

	if (add) {
		if (ubus_subscribe(&ubus, &ubus_subscriber, id)) {
			LOGN("ubus: failed to subscribe to %s (%u)", path, id);
			return;
		}
		LOGN("ubus: subscribe to %s (%u)", path, id);
		remote = malloc(sizeof(*remote));
		if (!remote) {
			LOG(ERR, "ubus: failed to alloc remote");
			return;
		}
		strncpy(remote->name, name, sizeof(remote->name));
		remote->avl.key = remote->name;
		remote->id = id;
		avl_insert(&ubus_tree, &remote->avl);
	} else {
		remote = avl_find_element(&ubus_tree, name, remote, avl);
		if (!remote) {
			LOG(ERR, "ubus: failed to find remote: %s", name);
			return;
		}
		avl_delete(&ubus_tree, &remote->avl);
		free(remote);
	}

	if (ubus_instance->subscribed)
		ubus_instance->subscribed(path, id, add);
}

static void ubus_event_handler_cb(struct ubus_context *ctx,  struct ubus_event_handler *ev,
				  const char *type, struct blob_attr *msg)
{
	enum {
		EVENT_ID,
		EVENT_PATH,
		__EVENT_MAX
	};

	static const struct blobmsg_policy status_policy[__EVENT_MAX] = {
		[EVENT_ID] = { .name = "id", .type = BLOBMSG_TYPE_INT32 },
		[EVENT_PATH] = { .name = "path", .type = BLOBMSG_TYPE_STRING },
	};

	struct blob_attr *tb[__EVENT_MAX];
	int add = !strcmp("ubus.object.add", type);

	blobmsg_parse(status_policy, __EVENT_MAX, tb, blob_data(msg), blob_len(msg));

	if (!tb[EVENT_ID] || !tb[EVENT_PATH]) {
		LOGE("ubus: Received invalid notify");
		return;
	}

	ubus_handle_subscribe(blobmsg_get_string(tb[EVENT_PATH]), blobmsg_get_u32(tb[EVENT_ID]), add);
}

static struct ubus_event_handler ubus_event_handler = { .cb = ubus_event_handler_cb };

static void ubus_lookup_cb(struct ubus_context *ctx, struct ubus_object_data *obj,
			   void *priv)
{
	char *path = strdup(obj->path);

	ubus_handle_subscribe(path, obj->id, 1);
	free(path);
}

static void ubus_ev(struct ev_loop *ev, struct ev_io *io, int event)
{
	ubus_handle_event(&ubus);
}

static void ubus_reconnect_cb(void *arg);

static void ubus_connection_lost_cb(struct ubus_context *ctx)
{
	ev_io_stop(loop, &ubus_io);
	evsched_task(&ubus_reconnect_cb, NULL, EVSCHED_SEC(1));
}
/*
static void wait_event_cb(struct ubus_context *ctx, struct ubus_event_handler *ev,
			  const char *type, struct blob_attr *msg)
{
	static const struct blobmsg_policy policy = {
		"path", BLOBMSG_TYPE_STRING
	};
	char objname[64];
	const char *path;
	struct blob_attr *attr;

	struct wait_event_data *data = container_of(ev, struct wait_event_data, ev);
	snprintf(objname, sizeof(objname), "hostapd.%s", data->ifname);

	if (strcmp(type, "ubus.object.add") != 0)
		return;

	blobmsg_parse(&policy, 1, &attr, blob_data(msg), blob_len(msg));
	if (!attr)
		return;

	path = blobmsg_data(attr);
	LOGI("======%s: path %s=======", path, data->ifname);
	if (!strcmp(path, objname)) {
		LOGI("======%s: ifup %s=======", path, objname);
		data->ifup = true;
		uloop_end();
	}
}

static void wait_event_timeout(struct uloop_timeout *timeout)
{
	uloop_end();
}

static int wait_for_ifup(char *ifname, int timeout)
{
	struct wait_event_data data;
	int ret;
	data.timeout.cb = wait_event_timeout;
	data.ev.cb = wait_event_cb;
	data.ifup = false;
	strncpy(data.ifname, ifname, IF_NAMESIZE);

	uloop_init();
	ubus_add_uloop(ctx);

	ret = ubus_register_event_handler(ctx, &data.ev, "ubus.object.add");
	if (ret)
		return ret;

	uloop_timeout_set(&data.timeout, timeout * 1000);
	uloop_run();
	uloop_done();

	if (!data.ifup)
		return UBUS_STATUS_TIMEOUT;

	return ret;
}*/

struct wait_event_data {
	struct uloop_timeout timeout;
	struct ubus_event_handler ev;
	char **pending;
	int n_pending;
};

static void wait_check_object(struct wait_event_data *data, const char *path)
{
	int i;

	for (i = 0; i < data->n_pending; i++) {
		LOG(INFO, "====pending======== %s", data->pending[i]);
		if (strcmp(path, data->pending[i]) != 0)
			continue;

		data->n_pending--;
		if (i == data->n_pending)
			break;

		memmove(&data->pending[i], &data->pending[i + 1],
			(data->n_pending - i) * sizeof(*data->pending));
		i--;
	}

	if (!data->n_pending) {
		LOG(INFO, "====IF UP ALL========");
		uloop_end();
	}

}

static void wait_event_cb(struct ubus_context *ctx, struct ubus_event_handler *ev,
			  const char *type, struct blob_attr *msg)
{
	static const struct blobmsg_policy policy = {
		"path", BLOBMSG_TYPE_STRING
	};
	struct wait_event_data *data = container_of(ev, struct wait_event_data, ev);
	struct blob_attr *attr;
	const char *path;

	if (strcmp(type, "ubus.object.add") != 0)
		return;

	blobmsg_parse(&policy, 1, &attr, blob_data(msg), blob_len(msg));
	if (!attr)
		return;

	path = blobmsg_data(attr);
	wait_check_object(data, path);
}

static void wait_list_cb(struct ubus_context *ctx, struct ubus_object_data *obj, void *priv)
{
	struct wait_event_data *data = priv;

	wait_check_object(data, obj->path);
}


static void wait_timeout(struct uloop_timeout *timeout)
{
	uloop_end();
}

int wait_for_ubus_obj(char **objlist, int n_objs, int timeout)
{
	struct wait_event_data data = {
		.timeout.cb = wait_timeout,
		.ev.cb = wait_event_cb,
		.pending = objlist,
		.n_pending = n_objs,
	};
	int ret = 0;

	struct ubus_context *ctx = NULL;
	ctx = ubus_connect(NULL);

	if (!ctx) {
		goto out;
	}

	uloop_init();
	ubus_add_uloop(ctx);

	ret = ubus_register_event_handler(ctx, &data.ev, "ubus.object.add");
	if (ret)
		goto out;

	if (!data.n_pending)
		goto out;

	ret = ubus_lookup(ctx, NULL, wait_list_cb, &data);
	if (ret)
		goto out;

	if (!data.n_pending)
		goto out;

	uloop_timeout_set(&data.timeout, timeout * 1000);
	uloop_run();
	uloop_done();

	if (data.n_pending) {
		LOG(INFO, "====TIMEDOUT========");
		ret = UBUS_STATUS_TIMEOUT;
	}

out:
	if (ctx)
		free(ctx);
	return ret;
}

static void ubus_reconnect_cb(void *arg)
{
	if (ubus_connect_ctx(&ubus, NULL)) {
		LOG(ERR, "ubus: failed to connect to");
		evsched_task_reschedule_ms(EVSCHED_SEC(1));
		return;
	}
	LOG(INFO, "ubus: connected");

	ubus.connection_lost = ubus_connection_lost_cb;

	ev_io_init(&ubus_io, ubus_ev, ubus.sock.fd, EV_READ);
        ev_io_start(loop, &ubus_io);

	if (ubus_instance->connect)
		ubus_instance->connect(&ubus);

	if (!ubus_instance->list || !ubus_instance->len || !ubus_instance->notify)
		return;

	ubus_register_event_handler(&ubus, &ubus_event_handler, "ubus.object.add");
	ubus_register_event_handler(&ubus, &ubus_event_handler, "ubus.object.remove");

	ubus_subscriber.cb = ubus_instance->notify;
	if (ubus_register_subscriber(&ubus, &ubus_subscriber))
		LOGN("ubus: failed to register ubus subscriber");

	ubus_lookup(&ubus, NULL, ubus_lookup_cb, NULL);
}

int ubus_init(struct ubus_instance *instance, struct ev_loop *_loop)
{
	ubus_instance = instance;
	loop = _loop;
	evsched_task(&ubus_reconnect_cb, NULL, EVSCHED_SEC(2));

	return 0;
}
