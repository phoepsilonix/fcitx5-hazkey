import Dispatch
import Foundation
import KanaKanjiConverterModule

class HazkeyServer: SocketManagerDelegate {
    private let processManager: ProcessManager
    private let socketManager: SocketManager
    private let protocolHandler: ProtocolHandler
    private let state: HazkeyServerState

    private let runtimeDir: String
    private let uid: uid_t
    private let socketPath: String

    init() {
        // Initialize runtime paths
        self.runtimeDir = ProcessInfo.processInfo.environment["XDG_RUNTIME_DIR"] ?? "/tmp"
        self.uid = getuid()
        self.socketPath = "\(runtimeDir)/hazkey-server.\(uid).sock"

        // Initialize managers
        self.processManager = ProcessManager()
        self.socketManager = SocketManager(socketPath: socketPath)

        // Initialize server state
        self.state = HazkeyServerState()

        self.protocolHandler = ProtocolHandler(state: state)

        // Set delegate
        socketManager.delegate = self
    }

    func start() throws {
        processManager.parseCommandLineArguments()
        try processManager.checkExistingServer()
        try processManager.createPidFile()
        try? processManager.createInfoFile() // less important
        try socketManager.setupSocket()

        NSLog("start listening...")
        socketManager.startListening()

        let _ = state.saveLearningData()
        // Leave them to stabilize
        // processManager.removeInfoFile()
        // processManager.removePidFile()
    }

    func socketManager(_ manager: SocketManager, didReceiveData data: Data, from clientFd: Int32)
        -> Data
    {
        return protocolHandler.processProto(data: data)
    }

    func socketManager(_ manager: SocketManager, clientDidConnect clientFd: Int32) {}

    func socketManager(_ manager: SocketManager, clientDidDisconnect clientFd: Int32) {}
}
