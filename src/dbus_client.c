#include <gio/gio.h>
#include <stdio.h>
int main(int argc, char **argv) {
    if (argc != 2) { fprintf(stderr, "사용법: %s '알림 문장'\n", argv[0]); return 1; }
    GError *error = NULL;
    GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);
    if (!conn) { g_printerr("세션 버스 오류: %s\n",error->message); return 1; }
    GVariant *result = g_dbus_connection_call_sync(conn, "org.example.Lab2",
        "/org/example/Lab2", "org.example.Lab2.Notify", "Send",
        g_variant_new("(s)",argv[1]), G_VARIANT_TYPE("(s)"),
        G_DBUS_CALL_FLAGS_NONE, 5000, NULL, &error);
    if (!result) { g_printerr("호출 실패: %s\n",error->message); g_error_free(error); g_object_unref(conn); return 1; }
    const gchar *reply; g_variant_get(result,"(&s)",&reply);
    printf("서버 응답: %s\n", reply);
    fflush(stdout);
    g_variant_unref(result); g_object_unref(conn); return 0;
}
