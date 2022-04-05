import <iostream>;

import MainApplication;

int main(size_t argc, char* argv[]) {
    try {
	    Application::Setup (std::span<char*>{argv, argc});
    	Application::InitializeVk ();
    	Application::Run ();
    	Application::TerminateVk ();
    	Application::Cleanup ();
    } catch (std::exception &e) {
	    std::cerr << e.what() << std::endl;
        return 1;
    }
}
