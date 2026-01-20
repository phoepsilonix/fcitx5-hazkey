import Dispatch
import Foundation
import KanaKanjiConverterModule

class HazkeyServer: SocketManagerDelegate {
    private let processManager: ProcessManager
    private let socketManager: SocketManager
    private let protocolHandler: ProtocolHandler
    private let llamaAvailable: Bool
    private let ggmlBackendDevices: [GGMLBackendDevice]
    private let state: HazkeyServerState

    private let runtimeDir: String
    private let uid: uid_t
    private let socketPath: String

    init() {
        // Initialize runtime paths
        self.runtimeDir = ProcessInfo.processInfo.environment["XDG_RUNTIME_DIR"] ?? "/tmp"
        self.uid = getuid()
        self.socketPath = "\(runtimeDir)/hazkey-server.\(uid).sock"

        var ggmlBackendDirectory =
            ProcessInfo.processInfo.environment["GGML_BACKEND_DIR"]
            ?? (systemLibraryPath + "/libllama/backends/")
        // trailing slash is important
        if !ggmlBackendDirectory.hasSuffix("/") {
            ggmlBackendDirectory.append("/")
        }
        loadGGMLBackends(from: ggmlBackendDirectory)

        ggmlBackendDevices = enumerateGGMLBackendDevices()
        #if DEBUG
            for device in ggmlBackendDevices {
                NSLog(
                    "GGML Backend Device: \(device.name), Type: \(device.type), Description: \(device.description)"
                )
            }
        #endif
        self.llamaAvailable = ggmlBackendDevices.count > 0

        // Initialize server state
        self.state = HazkeyServerState(ggmlBackendDevices: ggmlBackendDevices)

        // Initialize managers
        self.processManager = ProcessManager()
        self.socketManager = SocketManager(socketPath: socketPath)
        self.protocolHandler = ProtocolHandler(state: state)

        // Set delegate
        socketManager.delegate = self
    }

    func start() throws {
        processManager.parseCommandLineArguments()
        processManager.setupSignalHandlers()
        try processManager.checkExistingServer()
        try processManager.createPidFile()
        try socketManager.setupSocket()

        NSLog("start listening...")
        socketManager.startListening()
    }

    func socketManager(_ manager: SocketManager, didReceiveData data: Data, from clientFd: Int32)
        -> Data
    {
        return protocolHandler.processProto(data: data)
    }

    func socketManager(_ manager: SocketManager, clientDidConnect clientFd: Int32) {}

    func socketManager(_ manager: SocketManager, clientDidDisconnect clientFd: Int32) {}
}
