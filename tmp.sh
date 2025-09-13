PKG=org.qtproject.example.PastViewer
ADB="$ANDROID_SDK_ROOT/platform-tools/adb"
LLDB_SERVER="$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/darwin-x86_64/lib/clang/17.0.2/lib/linux/aarch64/lldb-server"

$ADB push "$LLDB_SERVER" /data/local/tmp/lldb-server >/dev/null
PID="$($ADB shell pidof -s "$PKG" | tr -d '\r')"; [ -z "$PID" ] && { echo "Start the app first"; exit 1; }
$ADB shell run-as "$PKG" sh -c 'mkdir -p code_cache/.lldb && cp /data/local/tmp/lldb-server code_cache/.lldb/lldb-server && chmod 700 code_cache/.lldb/lldb-server'
$ADB forward --remove tcp:5039 2>/dev/null || true
$ADB forward tcp:5039 tcp:5039
$ADB shell run-as "$PKG" sh -c "killall lldb-server 2>/dev/null || true; nohup code_cache/.lldb/lldb-server g :5039 --attach $PID >/dev/null 2>&1 &"
lldb -o "platform select remote-android" -o "log enable gdb-remote packets process" -o "settings set plugin.process.gdb-remote.packet-timeout 5" -o "gdb-remote 127.0.0.1:5039"