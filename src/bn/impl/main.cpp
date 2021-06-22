#include <app_base/Application.h>

#include <bn/impl/BNApp.h>

app_base::Application* app_base::Application::Create(int argc, char** argv,
                                 const std::shared_ptr<app_base::PlatformData>& platformData) {
  return new beacon::impl::BNApp(argc, argv, platformData);
}
