#include <app_base/Application.h>

#include "LgApp.h"

using namespace sk_app;

Application* Application::Create(
                                 int argc,
                                 char** argv,
                                 const std::shared_ptr<PlatformData>& platformData) {
  return new LgApp(argc, argv, platformData);
}
