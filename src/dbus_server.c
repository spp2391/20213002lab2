#include <gio/gio.h>
#include <stdio.h>
static const char xml[] =
"<node><interface name='org.example.Lab2.Notify'>"
"<method name='Send'><arg type='s' name='message' direction='in'/>"
"<arg type='s' name='reply' direction='out'/></method>"
"</interface></node>";
static GDBusNodeInfo *node;
static void called(GDBusConnection *connection, const gchar *sender,
                   const gchar *object_path, const gchar *interface_name,
                   const gchar *method_name, GVariant *parameters,
                   GDBusMethodInvocation *invocation, gpointer data) {
    (void)connection; (void)sender; (void)object_path; (void)interface_name; (void)data;
    if (g_strcmp0(method_name, "Send") == 0) {
        const gchar *message; g_variant_get(parameters, "(&s)", &message);
        printf("[서버 알림] %s\n", message);
	fflush(stdout);
        g_dbus_method_invocation_return_value(invocation, g_variant_new("(s)", "전달 완료"));
    }
}
static const GDBusInterfaceVTable vtable = {.method_call = called};
static void acquired(GDBusConnection *conn, const gchar *name, gpointer data) {
    (void)name; (void)data;
    GError *error = NULL;
    guint id = g_dbus_connection_register_object(conn, "/org/example/Lab2",
        node->interfaces[0], &vtable, NULL, NULL, &error);
    if (!id) { g_printerr("등록 실패: %s\n", error->message); g_error_free(error); }
    else printf("D-Bus 서버 준비 완료. 클라이언트 실행하세요.\n");
    fflush(stdout);
}
static void name_lost(GDBusConnection *conn, const gchar *name, gpointer data) {
    (void)conn; (void)data; g_printerr("D-Bus 이름을 사용할 수 없음: %s\n", name);
}
int main(void) {
    GError *error = NULL; node = g_dbus_node_info_new_for_xml(xml, &error);
    if (!node) { g_printerr("XML 오류: %s\n", error->message); return 1; }
    guint owner = g_bus_own_name(G_BUS_TYPE_SESSION, "org.example.Lab2",
        G_BUS_NAME_OWNER_FLAGS_NONE, NULL, acquired, name_lost, NULL, NULL);
    GMainLoop *loop = g_main_loop_new(NULL, FALSE); g_main_loop_run(loop);
    g_bus_unown_name(owner); g_main_loop_unref(loop); g_dbus_node_info_unref(node); return 0;
}
