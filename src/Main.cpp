
static void LogDebugMsg(const std::string message) {
    RE::ConsoleLog::GetSingleton()->Print(message.c_str());
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);

    // Start subprocess for save management after other mods are loaded
    SKSE::GetMessagingInterface()->RegisterListener([](SKSE::MessagingInterface::Message* message) {
        if (message->type == SKSE::MessagingInterface::kDataLoaded) {
            LogDebugMsg("SSM Loaded.");
        }
    });

    return true;
}