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
    // try {
	// 	Default::AppInstance::InitSingleton ();
	//     Default::AppInstance::Setup (std::span<char*>{argv, argc});
    // 	Default::AppInstance::InitializeVk ();
    // 	Default::AppInstance::Run ();
    // } catch (std::exception &e) {
	//     LOG_raw ("{:s}", e.what());
    //     return 1;
    // }
    // Default::AppInstance::TerminateVk ();
    // Default::AppInstance::Cleanup ();
}
