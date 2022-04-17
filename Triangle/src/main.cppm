#include <logging.hxx>

import MainApplication.MyApp;

int main(size_t argc, char* argv[]) 
{
    new MyApp::Instance;

    try {
        MyApp::Instance::Setup (std::span<char*>{argv, argc});
    	MyApp::Instance::InitializeVk ();
    	MyApp::Instance::Run ();
    } catch (std::exception &e) {
        LOG_raw ("{:s}", e.what());
        return 1;
    }
    MyApp::Instance::TerminateVk ();
    MyApp::Instance::Cleanup ();
}
