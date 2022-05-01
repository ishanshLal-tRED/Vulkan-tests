#include <logging.hxx>

import MainApplication.MyApp;
typedef MyApp::Instance APP;
int main(size_t argc, char* argv[]) 
{
    new APP;

    try {
        APP::Setup (std::span<char*>{argv, argc});
    	APP::InitializeVk ();
    	APP::Run ();
    } catch (std::exception &e) {
        LOG_raw ("{:s}", e.what());
        return 1;
    }
    APP::TerminateVk ();
    APP::Cleanup ();
}
