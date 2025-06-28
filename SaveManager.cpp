// Main script for SkyrimSaveManager

#include <thread>
#include <chrono>

void LogDebugMsg(const std::string message) {
    RE::ConsoleLog::GetSingleton()->Print(message.c_str());
}

void TestRunner() {
    LogDebugMsg("Hello from a thread!");
    std::this_thread::sleep_for(std::chrono::seconds(10));
    LogDebugMsg("Hello after 10 seconds!");
}

SKSEPluginLoad(const SKSE::LoadInterface *skse) {
    SKSE::Init(skse);

    // Start subprocess for save management after other mods are loaded
    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
        if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            std::thread task(TestRunner);
            task.detach();
        }
    });

    return true;
}