#include <logging.hxx>

import MainApplication;

int main(size_t argc, char* argv[]) {
    try {
	    Application::Setup (std::span<char*>{argv, argc});
    	Application::InitializeVk ();
    	Application::Run ();
    } catch (std::exception &e) {
	    LOG_raw ("{:s}", e.what());
        return 1;
    }
    Application::TerminateVk ();
    Application::Cleanup ();
}
