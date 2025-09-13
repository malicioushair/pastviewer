LLDB_SERVER="$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/darwin-x86_64/lib/clang/17.0.2/lib/linux/aarch64/lldb-server"

# Push it to a temp place
adb push "$LLDB_SERVER" /data/local/tmp/lldb-server >/dev/null

# Prepare an exec-friendly dir inside the app sandbox (quote carefully)
adb shell run-as "org.qtproject.example.PastViewer" 'mkdir -p code_cache/.lldb'
adb shell run-as "org.qtproject.example.PastViewer" 'cp /data/local/tmp/lldb-server code_cache/.lldb/lldb-server'
adb shell run-as "org.qtproject.example.PastViewer" 'chmod 700 code_cache/.lldb/lldb-server'
adb shell run-as "org.qtproject.example.PastViewer" 'ls -l code_cache/.lldb/lldb-server'