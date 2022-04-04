import MainApplication;

int main(size_t argc, char* argv[]) {
    Application::Setup (std::span<char*>{argv, argc});
    Application::InitializeVk ();
    Application::Run ();
    Application::TerminateVk ();
    Application::Cleanup ();
}