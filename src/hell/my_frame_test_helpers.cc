/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "my_frame_test_helpers.h"

#include <memory>
#include <utility>
#include <iostream>

#include "base/bind.h"
#include "cc/raster/single_thread_task_graph_runner.h"
#include "cc/trees/layer_tree_host.h"
#include "cc/trees/layer_tree_settings.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/common/frame/frame_policy.h"
#include "third_party/blink/public/platform/interface_registry.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_data.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url_loader_mock_factory.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/public/platform/web_url_response.h"
#include "third_party/blink/public/web/web_console_message.h"
#include "third_party/blink/public/web/web_frame_widget.h"
#include "third_party/blink/public/web/web_navigation_params.h"
#include "third_party/blink/public/web/web_settings.h"
#include "third_party/blink/public/web/web_tree_scope_type.h"
#include "third_party/blink/public/web/web_view_client.h"
#include "third_party/blink/renderer/core/exported/web_remote_frame_impl.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/testing/fake_web_plugin.h"
#include "third_party/blink/renderer/platform/heap/heap.h"
#include "third_party/blink/renderer/platform/loader/static_data_navigation_body_loader.h"
#include "third_party/blink/renderer/platform/scheduler/public/post_cross_thread_task.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread.h"
#include "third_party/blink/renderer/platform/scheduler/test/fake_task_runner.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/url_test_helpers.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"
#include "ui/events/keycodes/dom/dom_key.h"
#include "ui/events/keycodes/dom/keycode_converter.h"
#include "ui/events/blink/blink_event_util.h"

namespace SDK {

ResourceRequest::ResourceRequest(const std::string& url) : url(url) { }
ResourceRequest::~ResourceRequest() { }

ResourceResponse::ResourceResponse(const std::vector<uint8_t>& data,
                                 const std::string& mimeType)
    : data(data), mimeType(mimeType) { }

ResourceResponse::ResourceResponse()
    : data(std::vector<uint8_t>()), mimeType("text/html") { }

ResourceResponse::ResourceResponse(const ResourceResponse& other) = default;

ResourceResponse::~ResourceResponse() { }

ResourceResponse Backend::ProcessRequest(const ResourceRequest& request) {
  ResourceResponse response;

  if (request.getUrl() == "mem://logo.svg") {
    response.setMimeType("image/svg+xml");

    std::string svgText =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>   \
    <svg    \
        xmlns:dc=\"http://purl.org/dc/elements/1.1/\"    \
        xmlns:cc=\"http://creativecommons.org/ns#\"  \
        xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\"    \
        xmlns:svg=\"http://www.w3.org/2000/svg\" \
        xmlns=\"http://www.w3.org/2000/svg\" \
        id=\"svg8\"  \
        version=\"1.1\"  \
        viewBox=\"0 0 20 20\"    \
        height=\"20mm\"  \
        width=\"20mm\">  \
        <defs \
            id=\"defs2\" />    \
        <metadata \
            id=\"metadata5\">  \
        <rdf:RDF>   \
            <cc:Work  \
                rdf:about=\"\">    \
            <dc:format>image/svg+xml</dc:format>    \
            <dc:type    \
                rdf:resource=\"http://purl.org/dc/dcmitype/StillImage\" />   \
            <dc:title></dc:title>   \
            </cc:Work>    \
        </rdf:RDF>  \
        </metadata>   \
        <g    \
            transform=\"translate(0,-277)\"    \
            id=\"layer1\"> \
        <path   \
            d=\"m 6.3410945,291.73753 -5.1379613,-3.99351 v -0.59506 l 5.1379613,-3.98855 0.321704,0.71407 -4.5720358,3.56705 4.5720358,3.57697 z\"  \
            style=\"font-style:normal;font-variant:normal;font-weight:normal;font-stretch:normal;font-size:12.69999981px;line-height:1.25;font-family:'Century Schoolbook';-inkscape-font-specification:'Century Schoolbook';letter-spacing:0px;word-spacing:0px;fill:#000000;fill-opacity:1;stroke:none;stroke-width:0.21157649\"   \
            id=\"path14\" /> \
        <path   \
            d=\"M 11.82605,282.16947 9.0975343,292.56026 8.3780434,292.39934 11.11276,282.00855 Z\"  \
            style=\"font-style:normal;font-variant:normal;font-weight:normal;font-stretch:normal;font-size:12.69999981px;line-height:1.25;font-family:'Century Schoolbook';-inkscape-font-specification:'Century Schoolbook';letter-spacing:0px;word-spacing:0px;fill:#000000;fill-opacity:1;stroke:none;stroke-width:0.26458332\"   \
            id=\"path16\" /> \
        <path   \
            d=\"m 13.695671,291.73753 5.137962,-3.99351 v -0.59506 l -5.137962,-3.98855 -0.321703,0.71407 4.572036,3.56705 -4.572036,3.57697 z\" \
            style=\"font-style:normal;font-variant:normal;font-weight:normal;font-stretch:normal;font-size:12.69999981px;line-height:1.25;font-family:'Century Schoolbook';-inkscape-font-specification:'Century Schoolbook';letter-spacing:0px;word-spacing:0px;fill:#000000;fill-opacity:1;stroke:none;stroke-width:0.21157649\"   \
            id=\"path14-2\" />   \
        </g>  \
    </svg>  \
    ";

    std::vector<uint8_t> data(svgText.begin(), svgText.end());
    response.setData(data);

  } else if (request.getUrl() == "mem://index.html") {
    std::string htmlText =
    "    <html>"
    "    <head>"
    "        <title>Welcome to LightningUI</title>"
    "        <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />"
    "        <link rel=\"stylesheet\" href=\"mem://root.css\" />"
    "        <style>"
    "            @keyframes show {"
    "                0%   { max-height: 0pt; opacity: 0; visibility: hidden; }"
    "                1%   { max-height: 0pt; opacity: 0; visibility: visible; }"
    "                100% { max-height: 1000pt; opacity: 1; visibility: visible; }"
    "            }"
    "            @keyframes hide {"
    "                0% { max-height: 500pt; opacity: 1; visibility: visible; }"
    "                99%   { max-height: 0pt; opacity: 0; visibility: visible; }"
    "                100%   { max-height: 0pt; opacity: 0; visibility: hidden; }"
    "            }"
    "            .hidden { animation: hide 0.3s ease; animation-fill-mode: forwards; }"
    "            .shown { animation: show 0.3s ease; animation-fill-mode: forwards; }"
    "        </style>"
    "    </head>"
    "    <body>"
    "        <div class=\"logo\">"
    "            <img class=\"logo\" src=\"mem://logo.svg\"></img>"
    "            <div class=\"logo-title\">This is LightningUI</div>"
    "        </div>"
    "        <div class=\"content-frame\">"
    "            <div class=\"content-page\">"
    "                <div class=\"content-page-margins\">"
    "                    <p class=\"title-item\" onclick=\"toggle('description')\"><b>Description</b></p>"
    "                    <div id=\"description\" class=\"shown\">"
    "                        <p><b>LightningUI</b> is a framework, that provides <b>the developer</b> with a strong backend for flexible, fast and lightweight user interface development. As soon as <b>LightningUI</b> is based on the most popular HTML layout engine in the world called Blink, it has unlimited power under cover with the least excessive efforts possible.</p>"
    "                        <p><b>LightningUI</b> is a framework, that provides <b>the end-user</b> with a modern-looking and fast-paced application interaction experience.</p>" 
    "                        <p><b>LightningUI</b> is the fastest among all the HTML-based UI frameworks such as Electron, NW.js, "
    "                            CEF and everything else Chromium-based. If you don't believe that, just start resizing this window by dragging its corner randomly with the mouse and compare the visual sense to any application built upon another framework ;)"
    "                        </p>"
    "                    </div>"
    "                    <p class=\"title-item\" onclick=\"toggle('tests')\"><b>Basic tests</b></p>"
    "                    <div id=\"tests\" class=\"hidden\">"
    "                        <p id=\"jsout\">Message from JavaScript: </p>"
    "                        <script>document.getElementById(\"jsout\").innerHTML += \"<span> 3 + 2 = \" + (3+2) + \"</span>\";</script>"
    "                        <p>UI controls:</p>"
    "                        <button>I am a button!</button> <input type=\"submit\" value=\"Submit\"></input> <input type=\"text\" class=\"growing\" value=\"Hello!\"></input>"
    "                        <p>Link: <a href=\"mem://index.html\">I am a hyperref</a></p>"
    "                    </div>"
    "                </div>"
    "            </div>"
    "        </div>"
    "        <script>"
    "            function toggle(elementId) {"
    "                var element = document.getElementById(elementId);"
    "                if (element.classList.contains(\"hidden\")) {"
    "                    element.classList.remove(\"hidden\");"
    "                    element.classList.add(\"shown\");"
    "                } else {"
    "                    element.classList.remove(\"shown\");"
    "                    element.classList.add(\"hidden\");"
    "                }"
    "            }"
    "        </script>"
    "    </body>"
    "    </html>";

    std::vector<uint8_t> data(htmlText.begin(), htmlText.end());
    response.setData(data);
  } else if (request.getUrl() == "mem://root.css") {
    std::string htmlText =
    "html { "
    "    background: #eeeeee; "
    "    margin: 0; "
    "    font-size: 13px; "
    "    font-family: system-ui;"
    "} "

    "button, input[type=submit] {"
    "    -webkit-appearance: default-button; "
    "    color: black;"
    "} "

    "button:active, input[type=button]:active, input[type=submit]:active {"
    "    color: white;"
    "} "

    "input[type=text] {"
    "    -webkit-appearance: textfield;"
    "    outline-color: -webkit-focus-ring-color;"
    "} "

    "::selection { background: rgba(0, 0, 128, 0.2); } "

    "input[type=text].growing {"
    "   width: 100px;"
    "   transition: width .35s ease-in-out;"
    "} "
    
    "input[type=text].growing:focus {"
    "   width: 250px;"
    "} "

    "div.logo {"
    "    font-size: 25pt; "
    "    text-align: center; "
    "    margin-top: 10vh;"
    "}"

    "img.logo {"
    "    width: 80pt; "
    "    height: 80pt; "
    "    vertical-align: middle;"
    "}"

    "div.logo-title {"
    "    display: block;"
    "}"

    "div.content-frame {"
    "    overflow: auto;"
    "    position: absolute;"
    "    left: 0; right: 0; bottom: 0; top: calc(10vh + 120pt);"
    "}"

    "div.content-page {"
    "    max-width: 600pt; "
    "    margin: auto; "
    "    margin-top: 10pt;"
    "}"

    "div.content-page-margins {"
    "    margin: 0 10pt;"
    "}"

    "p.title-item {"
    "    margin-top: 5pt;"
    "    margin-bottom: 5pt;"
    "    font-size: 150%;"
    "}"

    "p.title-item::before {"
    "    content: '\\25B8 ';"
    "}"

    "@media only screen and (max-width: 400px) {"
    "    html { "
    "    }"
    "    div.logo {"
    "        font-size: 15pt; "
    "        margin-top: 0;"
    "    }"
    "    img.logo {"
    "        width: 45pt; "
    "        height: 45pt; "
    "        vertical-align: middle;"
    "    }"
    "    div.logo-title {"
    "        display: inline-block;"
    "    }"
    "    div.content-frame {"
    "        overflow: auto;"
    "        position: absolute;"
    "        left: 0; right: 0; bottom: 0; top: calc(60pt);"
    "    }"
    "}"

//    "::-webkit-scrollbar { width: 10px; }"
//    "::-webkit-scrollbar:hover { width: 15px; }"
//    "::-webkit-scrollbar-thumb { background: #888; border-radius: 10px; }"
//    "::-webkit-scrollbar-track { width: 15px; background: #ddd; border-radius: 0px; }"
;
        
        //"        /* Buttons */"
        //"::-webkit-scrollbar-button:single-button {"
        //"  background-color: #bbbbbb;"
        //"  display: block;"
        //"  border-style: solid;"
        //"  height: 20px;"
        //"  width: 20px;"
        //"}"
        //"/* Up */"
        //"::-webkit-scrollbar-button:single-button:vertical:decrement {"
        //"  content: \"@\";"
        //"}"
        //""
        //"::-webkit-scrollbar-button:single-button:vertical:decrement:hover {"
        //"  border-color: transparent transparent #777777 transparent;"
        //"}"
        //"/* Down */"
        //"::-webkit-scrollbar-button:single-button:vertical:increment {"
        //"  border-width: 8px 8px 0 8px;"
        //"  border-color: #555555 transparent transparent transparent;"
        //"}"
        //""
        //"::-webkit-scrollbar-button:vertical:single-button:increment:hover {"
        //"  border-color: #777777 transparent transparent transparent;"
        //"}";

    std::vector<uint8_t> data(htmlText.begin(), htmlText.end());
    response.setData(data);
  }

  return response;
}

Backend::Backend() { }
Backend::~Backend() { }

}  // namespace SDK

// We need this for WTF::Passed(std::move(blink::WebURLRequest))
namespace WTF {
template <>
struct CrossThreadCopier<blink::WebURLRequest>
    : public CrossThreadCopierPassThrough<blink::WebURLRequest> {
  STATIC_ONLY(CrossThreadCopier);
};

}  // namespace WTF


namespace blink {

/*
namespace my_scheduler {

class FakeTaskRunner::Data : public WTF::ThreadSafeRefCounted<Data> {
 public:
  Data() = default;

  void PostDelayedTask(base::OnceClosure task, base::TimeDelta delay) {
    task_queue_.emplace_back(std::move(task), time_ + delay);
  }

  using PendingTask = FakeTaskRunner::PendingTask;
  Deque<PendingTask>::iterator FindRunnableTask() {
    // TODO(tkent): This should return an item which has the minimum |second|.
    return std::find_if(
        task_queue_.begin(), task_queue_.end(),
        [&](const PendingTask& item) { return item.second <= time_; });
  }

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  Deque<PendingTask> task_queue_;
  base::TimeTicks time_;

 private:
  ~Data() = default;

  friend ThreadSafeRefCounted<Data>;
  DISALLOW_COPY_AND_ASSIGN(Data);
};

FakeTaskRunner::FakeTaskRunner() : data_(base::AdoptRef(new Data)) {}

FakeTaskRunner::FakeTaskRunner(scoped_refptr<Data> data)
    : data_(std::move(data)) {}

FakeTaskRunner::~FakeTaskRunner() = default;

void FakeTaskRunner::SetTime(base::TimeTicks new_time) {
  data_->time_ = new_time;
}

bool FakeTaskRunner::RunsTasksInCurrentSequence() const {
  return true;
}

void FakeTaskRunner::RunUntilIdle() {
  while (!data_->task_queue_.empty()) {
    // Move the task to run into a local variable in case it touches the
    // task queue by posting a new task.
    base::OnceClosure task = std::move(data_->task_queue_.front()).first;
    data_->task_queue_.pop_front();
    std::move(task).Run();
  }
}

void FakeTaskRunner::AdvanceTimeAndRun(base::TimeDelta delta) {
  data_->time_ += delta;
  for (auto it = data_->FindRunnableTask(); it != data_->task_queue_.end();
       it = data_->FindRunnableTask()) {
    base::OnceClosure task = std::move(*it).first;
    data_->task_queue_.erase(it);
    std::move(task).Run();
  }
}

Deque<std::pair<base::OnceClosure, base::TimeTicks>>
FakeTaskRunner::TakePendingTasksForTesting() {
  return std::move(data_->task_queue_);
}

bool FakeTaskRunner::PostDelayedTask(const base::Location& location,
                                     base::OnceClosure task,
                                     base::TimeDelta delay) {
  data_->PostDelayedTask(std::move(task), delay);
  return true;
}

bool FakeTaskRunner::PostNonNestableDelayedTask(const base::Location& location,
                                                base::OnceClosure task,
                                                base::TimeDelta delay) {
  data_->PostDelayedTask(std::move(task), delay);
  return true;
}

}  // namespace my_scheduler
*/
namespace my_frame_test_helpers {

namespace {

// The frame test helpers coordinate frame loads in a carefully choreographed
// dance. Since the parser is threaded, simply spinning the run loop once is not
// enough to ensure completion of a load. Instead, the following pattern is
// used to ensure that tests see the final state:
// 1. Starts a load.
// 2. Enter the run loop.
// 3. Posted task triggers the load, and starts pumping pending resource
//    requests using runServeAsyncRequestsTask().
// 4. TestWebFrameClient watches for DidStartLoading/DidStopLoading calls,
//    keeping track of how many loads it thinks are in flight.
// 5. While RunServeAsyncRequestsTask() observes TestWebFrameClient to still
//    have loads in progress, it posts itself back to the run loop.
// 6. When RunServeAsyncRequestsTask() notices there are no more loads in
//    progress, it exits the run loop.
// 7. At this point, all parsing, resource loads, and layout should be finished.

void RunServeAsyncRequestsTask(scoped_refptr<base::TaskRunner> task_runner) {
  // TODO(kinuko,toyoshim): Create a mock factory and use it instead of
  // getting the platform's one. (crbug.com/751425)
  // Platform::Current()->GetURLLoaderMockFactory()->ServeAsynchronousRequests();
  if (TestWebFrameClient::IsLoading()) {
    // task_runner->PostTask(FROM_HERE,
    //                      WTF::Bind(&RunServeAsyncRequestsTask, task_runner));
  } else {
    // test::ExitRunLoop();
  }
}

// Helper to create a default test client if the supplied client pointer is
// null. The |owned_client| is used to store the client if it must be created.
// In both cases the client to be used is returned.
template <typename T>
T* CreateDefaultClientIfNeeded(T* client, std::unique_ptr<T>& owned_client) {
  //if (client)
    return client;
  //owned_client = std::make_unique<T>();
  //return owned_client.get();
}

}  // namespace

void LoadFrameDontWait(WebLocalFrame* frame, const WebURL& url) {
  auto* impl = To<WebLocalFrameImpl>(frame);
  if (url.ProtocolIs("javascript")) {
    impl->LoadJavaScriptURL(url);
  } else {
    auto params = std::make_unique<WebNavigationParams>();
    params->url = url;
    params->navigation_timings.navigation_start = base::TimeTicks::Now();
    params->navigation_timings.fetch_start = base::TimeTicks::Now();
    params->is_browser_initiated = true;

    WebLocalFrameClient* client = frame->Client();
    TestWebFrameClient* tclient =
        static_cast<blink::my_frame_test_helpers::TestWebFrameClient*>(client);
    auto backend = tclient->Backend();
    FillNavigationParamsResponse(params.get(), backend.get());

    impl->CommitNavigation(
        std::move(params), nullptr /* extra_data */,
        base::DoNothing::Once() /* call_before_attaching_new_document */);
  }
}

void LoadFrame(WebLocalFrame* frame,
               const std::string& url) {
  LoadFrameDontWait(frame, url_test_helpers::ToKURL(url));
  PumpPendingRequestsForFrameToLoad(frame);
}

void LoadHTMLString(WebLocalFrame* frame,
                    const std::string& html,
                    const WebURL& base_url,
                    const base::TickClock* clock) {
  auto* impl = To<WebLocalFrameImpl>(frame);
  std::unique_ptr<WebNavigationParams> navigation_params =
      WebNavigationParams::CreateWithHTMLString(html, base_url);
  navigation_params->tick_clock = clock;
  impl->CommitNavigation(
      std::move(navigation_params), nullptr /* extra_data */,
      base::DoNothing::Once() /* call_before_attaching_new_document */);
  PumpPendingRequestsForFrameToLoad(frame);
}

void LoadHistoryItem(WebLocalFrame* frame, SDK::Backend* backend,
                     const WebHistoryItem& item,
                     mojom::FetchCacheMode cache_mode) {
  auto* impl = To<WebLocalFrameImpl>(frame);
  HistoryItem* history_item = item;
  auto params = std::make_unique<WebNavigationParams>();
  params->url = history_item->Url();
  params->frame_load_type = WebFrameLoadType::kBackForward;
  params->history_item = item;
  params->navigation_timings.navigation_start = base::TimeTicks::Now();
  params->navigation_timings.fetch_start = base::TimeTicks::Now();
  FillNavigationParamsResponse(params.get(), backend);
  impl->CommitNavigation(
      std::move(params), nullptr /* extra_data */,
      base::DoNothing::Once() /* call_before_attaching_new_document */);
  PumpPendingRequestsForFrameToLoad(frame);
}

void ReloadFrame(WebLocalFrame* frame) {
  frame->StartReload(WebFrameLoadType::kReload);
  PumpPendingRequestsForFrameToLoad(frame);
}

void ReloadFrameBypassingCache(WebLocalFrame* frame) {
  frame->StartReload(WebFrameLoadType::kReloadBypassingCache);
  PumpPendingRequestsForFrameToLoad(frame);
}

void PumpPendingRequestsForFrameToLoad(WebLocalFrame* frame) {
  scoped_refptr<base::TaskRunner> task_runner =
      frame->GetTaskRunner(blink::TaskType::kInternalTest);
  task_runner->PostTask(FROM_HERE,
                        WTF::Bind(&RunServeAsyncRequestsTask, task_runner));
  // test::EnterRunLoop();
}

void FillNavigationParamsResponse(WebNavigationParams* params,
                                  SDK::Backend* backend) {
  params->response = WebURLResponse(params->url);

  SDK::ResourceRequest resReq(params->url.GetString().Ascii());
  SDK::ResourceResponse resResp = backend->ProcessRequest(resReq);

  blink::WebString mimeType =
      blink::WebString::FromASCII(resResp.getMimeType());

  params->response.SetMimeType(mimeType);
  params->response.SetHttpStatusCode(200);

  auto body_loader = std::make_unique<blink::StaticDataNavigationBodyLoader>();
  body_loader->Write((const char*)&resResp.getData()[0],
                     resResp.getData().size());
  body_loader->Finish();
  params->body_loader = std::move(body_loader);
}

WebMouseEvent CreateMouseEvent(WebInputEvent::Type type,
                               WebMouseEvent::Button button,
                               const IntPoint& point,
                               int modifiers) {
  WebMouseEvent result(type, modifiers, base::TimeTicks());
  result.pointer_type = WebPointerProperties::PointerType::kMouse;
  result.SetPositionInWidget(point.X(), point.Y());
  result.SetPositionInScreen(point.X(), point.Y());
  result.button = button;
  result.click_count = 1;
  return result;
}

WebMouseWheelEvent CreateMouseWheelEvent(WebMouseWheelEvent::Phase phase, int delta_x,
    int delta_y,
    int modifiers) {

  auto type = WebInputEvent::Type::kMouseWheel;
  WebMouseWheelEvent result(type, modifiers, base::TimeTicks::Now());
  result.delta_x = delta_x;
  result.delta_y = delta_y;
  result.phase = phase;
  return result;
}


WebKeyboardEvent CreateKeyboardEvent(char key_code,
                                     int modifiers,
                                     WebInputEvent::Type type) {
  WebKeyboardEvent event(type, modifiers, base::TimeTicks());
  event.text[0] = key_code;
  event.windows_key_code = key_code;
  event.dom_key = ui::DomKey::FromCharacter(key_code); /*ui::KeycodeConverter::NativeKeycodeToDomCode(
      key_code); */ //
  return event;
}

WebLocalFrameImpl* CreateLocalChild(WebLocalFrame& parent,
                                    WebTreeScopeType scope,
                                    TestWebFrameClient* client) {
  std::unique_ptr<TestWebFrameClient> owned_client;
  client = CreateDefaultClientIfNeeded(client, owned_client);
  auto* frame =
      To<WebLocalFrameImpl>(parent.CreateLocalChild(scope, client, nullptr));
  client->Bind(frame, std::move(owned_client));
  return frame;
}

WebLocalFrameImpl* CreateLocalChild(
    WebLocalFrame& parent,
    WebTreeScopeType scope,
    std::unique_ptr<TestWebFrameClient> self_owned) {
  DCHECK(self_owned);
  TestWebFrameClient* client = self_owned.get();
  auto* frame =
      To<WebLocalFrameImpl>(parent.CreateLocalChild(scope, client, nullptr));
  client->Bind(frame, std::move(self_owned));
  return frame;
}

WebLocalFrameImpl* CreateProvisional(WebRemoteFrame& old_frame,
                                     TestWebFrameClient* client) {
  /*  std::unique_ptr<TestWebFrameClient> owned_client;
    client = CreateDefaultClientIfNeeded(client, owned_client);
    auto* frame = To<WebLocalFrameImpl>(WebLocalFrame::CreateProvisional(
        client, nullptr, &old_frame, FramePolicy()));
    client->Bind(frame, std::move(owned_client));
    std::unique_ptr<TestWebWidgetClient> widget_client;
    // Create a local root, if necessary.
    if (!frame->Parent()) {
      widget_client = std::make_unique<TestWebWidgetClient>();
      // TODO(dcheng): The main frame widget currently has a special case.
      // Eliminate this once WebView is no longer a WebWidget.
      WebFrameWidget::CreateForMainFrame(widget_client.get(), frame);
    } else if (frame->Parent()->IsWebRemoteFrame()) {
      widget_client = std::make_unique<TestWebWidgetClient>();
      WebFrameWidget* frame_widget =
          WebFrameWidget::CreateForChildLocalRoot(widget_client.get(), frame);
      // The WebWidget requires an AnimationHost to be set, either by the
      // WebWidgetClient itself or by someone else. We do that here.
     // frame_widget->SetAnimationHost(widget_client->animation_host());
      frame_widget->Resize(WebSize());
    }
    if (widget_client)
      client->BindWidgetClient(std::move(widget_client));
    return frame;*/
  return nullptr;
}

WebRemoteFrameImpl* CreateRemote(TestWebRemoteFrameClient* client) {
  std::unique_ptr<TestWebRemoteFrameClient> owned_client;
  client = CreateDefaultClientIfNeeded(client, owned_client);
  auto* frame = MakeGarbageCollected<WebRemoteFrameImpl>(
      WebTreeScopeType::kDocument, client,
      InterfaceRegistry::GetEmptyInterfaceRegistry(),
      client->GetAssociatedInterfaceProvider());
  client->Bind(frame, std::move(owned_client));
  return frame;
}

WebLocalFrameImpl* CreateLocalChild(WebRemoteFrame& parent,
                                    const WebString& name,
                                    const WebFrameOwnerProperties& properties,
                                    WebFrame* previous_sibling,
                                    TestWebFrameClient* client,
                                    TestWebWidgetClient* widget_client) {
  std::unique_ptr<TestWebFrameClient> owned_client;
  client = CreateDefaultClientIfNeeded(client, owned_client);
  auto* frame = To<WebLocalFrameImpl>(parent.CreateLocalChild(
      WebTreeScopeType::kDocument, name, FramePolicy(), client, nullptr,
      previous_sibling, properties, FrameOwnerElementType::kIframe, nullptr));
  client->Bind(frame, std::move(owned_client));

  std::unique_ptr<TestWebWidgetClient> owned_widget_client;
  widget_client =
      CreateDefaultClientIfNeeded(widget_client, owned_widget_client);
  WebFrameWidget* frame_widget =
      WebFrameWidget::CreateForChildLocalRoot(widget_client, frame);
  // The WebWidget requires an AnimationHost to be set, either by the
  // WebWidgetClient itself or by someone else. We do that here.
  // frame_widget->SetAnimationHost(widget_client->animation_host());
  // Set an initial size for subframes.
  if (frame->Parent())
    frame_widget->Resize(WebSize());
  client->BindWidgetClient(std::move(owned_widget_client));
  return frame;
}

WebRemoteFrameImpl* CreateRemoteChild(
    WebRemoteFrame& parent,
    const WebString& name,
    scoped_refptr<SecurityOrigin> security_origin,
    TestWebRemoteFrameClient* client) {
  std::unique_ptr<TestWebRemoteFrameClient> owned_client;
  client = CreateDefaultClientIfNeeded(client, owned_client);
  auto* frame = ToWebRemoteFrameImpl(parent.CreateRemoteChild(
      WebTreeScopeType::kDocument, name, FramePolicy(),
      FrameOwnerElementType::kIframe, client,
      InterfaceRegistry::GetEmptyInterfaceRegistry(),
      client->GetAssociatedInterfaceProvider(), nullptr));
  client->Bind(frame, std::move(owned_client));
  if (!security_origin)
    security_origin = SecurityOrigin::CreateUniqueOpaque();
  frame->GetFrame()->SetReplicatedOrigin(std::move(security_origin), false);
  return frame;
}

WebViewHelper::WebViewHelper()
    : web_view_(nullptr), platform_(Platform::Current()) {}

WebViewHelper::~WebViewHelper() {
  // Close the WebViewImpl before the WebViewClient/WebWidgetClient are
  // destroyed.
  Reset();
}

WebViewImpl* WebViewHelper::InitializeWithOpener(
    WebFrame* opener,
    TestWebFrameClient* web_frame_client,
    TestWebViewClient* web_view_client,
    TestWebWidgetClient* web_widget_client,
    void (*update_settings_func)(WebSettings*)) {
  Reset();

  InitializeWebView(web_view_client, opener ? opener->View() : nullptr);
  if (update_settings_func)
    update_settings_func(web_view_->GetSettings());

  std::unique_ptr<TestWebFrameClient> owned_web_frame_client;
  web_frame_client =
      CreateDefaultClientIfNeeded(web_frame_client, owned_web_frame_client);
  WebLocalFrame* frame = WebLocalFrame::CreateMainFrame(
      web_view_, web_frame_client, nullptr, opener);
  web_frame_client->Bind(frame, std::move(owned_web_frame_client));

  test_web_widget_client_ = CreateDefaultClientIfNeeded(
      web_widget_client, owned_test_web_widget_client_);
  // TODO(danakj): Make this part of attaching the main frame's WebFrameWidget.
  // This happens before CreateForMainFrame as the WebFrameWidget binding to the
  // WebLocalFrameImpl sets up animations.

  // web_view_->SetAnimationHost(test_web_widget_client_->animation_host());

  // TODO(dcheng): The main frame widget currently has a special case.
  // Eliminate this once WebView is no longer a WebWidget.
  blink::WebFrameWidget::CreateForMainFrame(test_web_widget_client_, frame);
  // We inform the WebView when it has a local main frame attached once the
  // WebFrame it fully set up and the WebWidgetClient is initialized (which is
  // the case by this point).
  web_view_->DidAttachLocalMainFrame();

  web_view_->SetDeviceScaleFactor(
      test_web_widget_client_->GetScreenInfo().device_scale_factor);

  // Set an initial size for subframes.
  if (frame->Parent())
    frame->FrameWidget()->Resize(WebSize());

  return web_view_;
}

WebViewImpl* WebViewHelper::Initialize(
    TestWebFrameClient* web_frame_client,
    TestWebViewClient* web_view_client,
    TestWebWidgetClient* web_widget_client,
    void (*update_settings_func)(WebSettings*)) {
  return InitializeWithOpener(nullptr, web_frame_client, web_view_client,
                              web_widget_client, update_settings_func);
}

WebViewImpl* WebViewHelper::InitializeWithSettings(
    void (*update_settings_func)(WebSettings*)) {
  return InitializeWithOpener(nullptr, nullptr, nullptr, nullptr,
                              update_settings_func);
}

WebViewImpl* WebViewHelper::InitializeAndLoad(
    const std::string& url,
    TestWebFrameClient* web_frame_client,
    TestWebViewClient* web_view_client,
    TestWebWidgetClient* web_widget_client,
    void (*update_settings_func)(WebSettings*)) {
  Initialize(web_frame_client, web_view_client, web_widget_client,
             update_settings_func);

  LoadFrame(GetWebView()->MainFrameImpl(), url);

  return GetWebView();
}

WebViewImpl* WebViewHelper::InitializeRemote(
    TestWebRemoteFrameClient* client,
    scoped_refptr<SecurityOrigin> security_origin,
    TestWebViewClient* web_view_client,
    TestWebWidgetClient* web_widget_client) {
  return InitializeRemoteWithOpener(nullptr, client, security_origin,
                                    web_view_client, web_widget_client);
}

WebViewImpl* WebViewHelper::InitializeRemoteWithOpener(
    WebFrame* opener,
    TestWebRemoteFrameClient* web_remote_frame_client,
    scoped_refptr<SecurityOrigin> security_origin,
    TestWebViewClient* web_view_client,
    TestWebWidgetClient* web_widget_client) {
  Reset();

  InitializeWebView(web_view_client, nullptr);

  std::unique_ptr<TestWebRemoteFrameClient> owned_web_remote_frame_client;
  web_remote_frame_client = CreateDefaultClientIfNeeded(
      web_remote_frame_client, owned_web_remote_frame_client);
  WebRemoteFrameImpl* frame = WebRemoteFrameImpl::CreateMainFrame(
      web_view_, web_remote_frame_client,
      InterfaceRegistry::GetEmptyInterfaceRegistry(),
      web_remote_frame_client->GetAssociatedInterfaceProvider(), opener);
  web_remote_frame_client->Bind(frame,
                                std::move(owned_web_remote_frame_client));
  if (!security_origin)
    security_origin = SecurityOrigin::CreateUniqueOpaque();
  frame->GetFrame()->SetReplicatedOrigin(std::move(security_origin), false);

  test_web_widget_client_ = CreateDefaultClientIfNeeded(
      web_widget_client, owned_test_web_widget_client_);
  // web_view_->SetAnimationHost(test_web_widget_client_->animation_host());

  return web_view_;
}

void WebViewHelper::LoadAhem() {
  LocalFrame* local_frame =
      To<LocalFrame>(WebFrame::ToCoreFrame(*LocalMainFrame()));
  DCHECK(local_frame);

  // TODO Enable it somehow
  // RenderingTest::LoadAhem(*local_frame);
}

void WebViewHelper::Reset() {
  DCHECK_EQ(platform_, Platform::Current())
      << "Platform::Current() should be the same for the life of a test, "
         "including shutdown.";

  if (test_web_view_client_)
    test_web_view_client_->DestroyChildViews();
  if (web_view_) {
    DCHECK(!TestWebFrameClient::IsLoading());
    web_view_->Close();
    web_view_ = nullptr;
  }
  test_web_view_client_ = nullptr;
}

WebLocalFrameImpl* WebViewHelper::LocalMainFrame() const {
  return To<WebLocalFrameImpl>(web_view_->MainFrame());
}

WebRemoteFrameImpl* WebViewHelper::RemoteMainFrame() const {
  return ToWebRemoteFrameImpl(web_view_->MainFrame());
}

void WebViewHelper::Resize(WebSize size) {
  GetWebView()->Resize(size);
}

void WebViewHelper::InitializeWebView(TestWebViewClient* web_view_client,
                                      class WebView* opener) {
  test_web_view_client_ =
      CreateDefaultClientIfNeeded(web_view_client, owned_test_web_view_client_);
  web_view_ = static_cast<WebViewImpl*>(
      WebView::Create(test_web_view_client_,
                      /*is_hidden=*/false,
                      /*compositing_enabled=*/true, opener));
  // This property must be set at initialization time, it is not supported to be
  // changed afterward, and does nothing.
  web_view_->GetSettings()->SetViewportEnabled(viewport_enabled_);
  web_view_->GetSettings()->SetJavaScriptEnabled(true);
  web_view_->GetSettings()->SetPluginsEnabled(true);

  // Enable (mocked) network loads of image URLs, as this simplifies
  // the completion of resource loads upon test shutdown & helps avoid
  // dormant loads trigger Resource leaks for image loads.
  //
  // Consequently, all external image resources must be mocked.
  web_view_->GetSettings()->SetLoadsImagesAutomatically(true);

  // If a test turned off this settings, opened WebViews should propagate that.
  if (opener) {
    web_view_->GetSettings()->SetAllowUniversalAccessFromFileURLs(
        static_cast<WebViewImpl*>(opener)
            ->GetPage()
            ->GetSettings()
            .GetAllowUniversalAccessFromFileURLs());
  }

  web_view_->SetDefaultPageScaleLimits(1, 4);
}

int TestWebFrameClient::loads_in_progress_ = 0;

TestWebFrameClient::TestWebFrameClient(std::shared_ptr<SDK::Backend> backend)
    : backend(backend), interface_provider_(
          new service_manager::InterfaceProvider()),
      associated_interface_provider_(new AssociatedInterfaceProvider(nullptr)),
      effective_connection_type_(WebEffectiveConnectionType::kTypeUnknown) {}

void TestWebFrameClient::SetScheduler(
    std::shared_ptr<blink::scheduler::WebThreadScheduler> my_web_thread_sched) {
  this->my_web_thread_sched = my_web_thread_sched;
}

TestWebFrameClient::~TestWebFrameClient() = default;

void TestWebFrameClient::Bind(WebLocalFrame* frame,
                              std::unique_ptr<TestWebFrameClient> self_owned) {
  DCHECK(!frame_);
  DCHECK(!self_owned || self_owned.get() == this);
  frame_ = To<WebLocalFrameImpl>(frame);
  self_owned_ = std::move(self_owned);
}

void TestWebFrameClient::BindWidgetClient(
    std::unique_ptr<WebWidgetClient> client) {
  DCHECK(!owned_widget_client_);
  owned_widget_client_ = std::move(client);
}

void TestWebFrameClient::FrameDetached(DetachType type) {
  if (frame_->FrameWidget())
    frame_->FrameWidget()->Close();

  owned_widget_client_.reset();
  frame_->Close();
  self_owned_.reset();
}

WebLocalFrame* TestWebFrameClient::CreateChildFrame(
    WebLocalFrame* parent,
    WebTreeScopeType scope,
    const WebString& name,
    const WebString& fallback_name,
    const FramePolicy&,
    const WebFrameOwnerProperties& frame_owner_properties,
    FrameOwnerElementType owner_type) {
  return CreateLocalChild(*parent, scope);
}

void TestWebFrameClient::DidStartLoading() {
  ++loads_in_progress_;
}

void TestWebFrameClient::DidStopLoading() {
  DCHECK_GT(loads_in_progress_, 0);
  --loads_in_progress_;
}

void TestWebFrameClient::BeginNavigation(
    std::unique_ptr<WebNavigationInfo> info) {
  navigation_callback_.Cancel();
  if (DocumentLoader::WillLoadUrlAsEmpty(info->url_request.Url()) &&
      !frame_->HasCommittedFirstRealLoad()) {
    CommitNavigation(std::move(info));
    return;
  }

  if (!frame_->WillStartNavigation(
          *info, false /* is_history_navigation_in_new_child_frame */))
    return;

  navigation_callback_.Reset(
      base::BindOnce(&TestWebFrameClient::CommitNavigation,
                     weak_factory_.GetWeakPtr(), std::move(info)));
  frame_->GetTaskRunner(blink::TaskType::kInternalLoading)
      ->PostTask(FROM_HERE, navigation_callback_.callback());
}

void TestWebFrameClient::CommitNavigation(
    std::unique_ptr<WebNavigationInfo> info) {
  if (!frame_)
    return;
  auto params = WebNavigationParams::CreateFromInfo(*info);
  if (info->archive_status != WebNavigationInfo::ArchiveStatus::Present)
    FillNavigationParamsResponse(params.get(), backend.get());
  frame_->CommitNavigation(
      std::move(params), nullptr /* extra_data */,
      base::DoNothing::Once() /* call_before_attaching_new_document */);
}

WebEffectiveConnectionType TestWebFrameClient::GetEffectiveConnectionType() {
  return effective_connection_type_;
}

void TestWebFrameClient::SetEffectiveConnectionTypeForTesting(
    WebEffectiveConnectionType effective_connection_type) {
  effective_connection_type_ = effective_connection_type;
}

void TestWebFrameClient::DidAddMessageToConsole(
    const WebConsoleMessage& message,
    const WebString& source_name,
    unsigned source_line,
    const WebString& stack_trace) {

    std::cout << message.url.Utf8() << " (ln " << message.line_number << ", col "
            << message.column_number << "): " << message.text.Utf8()
            << std::endl;

  //console_messages_.push_back(message.text);
}

WebPlugin* TestWebFrameClient::CreatePlugin(const WebPluginParams& params) {
  // TODO: Support FakeWebPlugin if necessary
  return nullptr;  // new FakeWebPlugin(params);
}

AssociatedInterfaceProvider*
TestWebFrameClient::GetRemoteNavigationAssociatedInterfaces() {
  return associated_interface_provider_.get();
}

TestWebRemoteFrameClient::TestWebRemoteFrameClient()
    : associated_interface_provider_(new AssociatedInterfaceProvider(nullptr)) {
}

void TestWebRemoteFrameClient::Bind(
    WebRemoteFrame* frame,
    std::unique_ptr<TestWebRemoteFrameClient> self_owned) {
  DCHECK(!frame_);
  DCHECK(!self_owned || self_owned.get() == this);
  frame_ = frame;
  self_owned_ = std::move(self_owned);
}

void TestWebRemoteFrameClient::FrameDetached(DetachType type) {
  frame_->Close();
  self_owned_.reset();
}

TestUkmRecorderFactory::~TestUkmRecorderFactory() = default;

std::unique_ptr<ukm::UkmRecorder> TestUkmRecorderFactory::CreateRecorder() {
  return std::make_unique<ukm::UkmRecorderImpl>();
}

void StubLayerTreeViewDelegate::RequestNewLayerTreeFrameSink(
    LayerTreeFrameSinkCallback callback) {
  std::move(callback).Run(nullptr);
}

std::unique_ptr<cc::BeginMainFrameMetrics>
StubLayerTreeViewDelegate::GetBeginMainFrameMetrics() {
  return nullptr;
}

TestWebWidgetClient::TestWebWidgetClient(
    content::LayerTreeViewDelegate* delegate,
    scoped_refptr<base::SingleThreadTaskRunner> mainTaskRunner,
    scoped_refptr<base::SingleThreadTaskRunner> composeTaskRunner,
    blink::scheduler::WebThreadScheduler* my_web_thread_sched) {
  layer_tree_view_ = std::make_unique<content::LayerTreeView>(delegate, mainTaskRunner,
      composeTaskRunner, new cc::SingleThreadTaskGraphRunner(), my_web_thread_sched);

  cc::LayerTreeSettings settings;
  //  Use synchronous compositing so that the MessageLoop becomes idle and
  //  the test makes progress.
  settings.single_thread_proxy_scheduler = true;
  settings.using_synchronous_renderer_compositor = true;
  settings.use_layer_lists = true;

  layer_tree_view_->Initialize(settings,
                               std::make_unique<TestUkmRecorderFactory>());

  animation_host_ = layer_tree_view_->animation_host();
}

void TestWebWidgetClient::SetRootLayer(scoped_refptr<cc::Layer> layer) {
  layer_tree_host()->SetRootLayer(std::move(layer));
}

void TestWebWidgetClient::SetBackgroundColor(SkColor color) {
  layer_tree_host()->set_background_color(color);
}

void TestWebWidgetClient::SetPageScaleStateAndLimits(
    float page_scale_factor,
    bool is_pinch_gesture_active,
    float minimum, float maximum) {
  layer_tree_host()->SetPageScaleFactorAndLimits(page_scale_factor, minimum,
                                                 maximum);
}

void TestWebWidgetClient::InjectGestureScrollEvent(
    WebGestureDevice device,
    const WebFloatSize& delta,
    ScrollGranularity granularity,
    cc::ElementId scrollable_area_element_id,
    WebInputEvent::Type injected_type) {
  InjectedScrollGestureData data{delta, granularity, scrollable_area_element_id,
                                 injected_type};
  injected_scroll_gesture_data_.push_back(data);

}

void TestWebWidgetClient::HandleScrollEvents(WebWidget* widget) {
  // Scrolling by pulling the scrollbar by hand
  for (auto& data : injected_scroll_gesture_data_) {
    base::TimeTicks now = base::TimeTicks::Now();
    std::unique_ptr<WebGestureEvent> gesture_event =
        ui::GenerateInjectedScrollGesture(
            data.type, now, WebGestureDevice::kSyntheticAutoscroll,
            gfx::PointF(0, 0),
            gfx::Vector2dF(data.delta.width, data.delta.height),
            data.granularity);
    if (data.type == WebInputEvent::Type::kGestureScrollBegin) {
      gesture_event->data.scroll_begin.scrollable_area_element_id =
          data.scrollable_area_element_id.GetStableId();
    }

    widget->HandleInputEvent(WebCoalescedInputEvent(*gesture_event));
  }
  injected_scroll_gesture_data_.clear();
}


void TestWebWidgetClient::SetHaveScrollEventHandlers(bool have_handlers) {
  have_scroll_event_handlers_ = have_handlers;
}

void TestWebWidgetClient::SetEventListenerProperties(
    cc::EventListenerClass event_class,
    cc::EventListenerProperties properties) {
  layer_tree_host()->SetEventListenerProperties(event_class, properties);
}

cc::EventListenerProperties TestWebWidgetClient::EventListenerProperties(
    cc::EventListenerClass event_class) const {
  return layer_tree_host()->event_listener_properties(event_class);
}

std::unique_ptr<cc::ScopedDeferMainFrameUpdate>
TestWebWidgetClient::DeferMainFrameUpdate() {
  return layer_tree_host()->DeferMainFrameUpdate();
}

void TestWebWidgetClient::StartDeferringCommits(base::TimeDelta timeout) {
  layer_tree_host()->StartDeferringCommits(timeout);
}

void TestWebWidgetClient::StopDeferringCommits(
    cc::PaintHoldingCommitTrigger trigger) {
  layer_tree_host()->StopDeferringCommits(trigger);
}

void TestWebWidgetClient::RegisterSelection(
    const cc::LayerSelection& selection) {
  layer_tree_host()->RegisterSelection(selection);
}

void TestWebWidgetClient::DidMeaningfulLayout(
    WebMeaningfulLayout meaningful_layout) {
  switch (meaningful_layout) {
    case WebMeaningfulLayout::kVisuallyNonEmpty:
      visually_non_empty_layout_count_++;
      break;
    case WebMeaningfulLayout::kFinishedParsing:
      finished_parsing_layout_count_++;
      break;
    case WebMeaningfulLayout::kFinishedLoading:
      finished_loading_layout_count_++;
      break;
  }
}

void TestWebWidgetClient::SetBrowserControlsShownRatio(float top_ratio,
                                                       float bottom_ratio) {
  layer_tree_host()->SetBrowserControlsShownRatio(top_ratio, bottom_ratio);
}

void TestWebWidgetClient::SetBrowserControlsParams(
    cc::BrowserControlsParams params) {
  layer_tree_host()->SetBrowserControlsParams(params);
}

viz::FrameSinkId TestWebWidgetClient::GetFrameSinkId() {
  return viz::FrameSinkId();
}

void TestWebViewClient::DestroyChildViews() {
  child_web_views_.clear();
}

WebView* TestWebViewClient::CreateView(WebLocalFrame* opener,
                                       const WebURLRequest&,
                                       const WebWindowFeatures&,
                                       const WebString& name,
                                       WebNavigationPolicy,
                                       WebSandboxFlags,
                                       const FeaturePolicy::FeatureState&,
                                       const SessionStorageNamespaceId&) {
  auto webview_helper = std::make_unique<WebViewHelper>();
  WebView* result = webview_helper->InitializeWithOpener(opener);
  child_web_views_.push_back(std::move(webview_helper));
  return result;
}

TestWebWidgetClient::~TestWebWidgetClient() {}
void TestWebWidgetClient::ScheduleAnimation() {
  animation_scheduled_ = true;
}
TestWebViewClient::TestWebViewClient() {}
TestWebViewClient::~TestWebViewClient() {}

bool TestWebViewClient::CanHandleGestureEvent() {
  return true;
}
bool TestWebViewClient::CanUpdateLayout() {
  return true;
}
TestWebRemoteFrameClient::~TestWebRemoteFrameClient() {}

service_manager::InterfaceProvider* TestWebFrameClient::GetInterfaceProvider() {
  return interface_provider_.get();
}
std::unique_ptr<blink::WebURLLoaderFactory>
TestWebFrameClient::CreateURLLoaderFactory() {
  // TODO(kinuko,toyoshim): Stop using Platform's URLLoaderFactory, but create
  // its own WebURLLoaderFactoryWithMock. (crbug.com/751425)
  return std::make_unique<MyWebURLLoaderFactory>(my_web_thread_sched, backend);
  // Platform::Current()->CreateDefaultURLLoaderFactory();
}
WebRemoteFrame* TestWebRemoteFrameClient::Frame() const {
  return frame_;
}

MyWebURLLoader::MyWebURLLoader(
    std::shared_ptr<blink::scheduler::WebThreadScheduler> my_web_thread_sched, std::shared_ptr<SDK::Backend> backend)
    : my_web_thread_sched(my_web_thread_sched), backend(backend) {}

MyWebURLLoader::~MyWebURLLoader() {}

void MyWebURLLoader::LoadSynchronously(const WebURLRequest&,
                                       WebURLLoaderClient*,
                                       WebURLResponse&,
                                       base::Optional<WebURLError>&,
                                       WebData&,
                                       int64_t& encoded_data_length,
                                       int64_t& encoded_body_length,
                                       WebBlobInfo& downloaded_blob) {
  NOTREACHED();
}

void MyWebURLLoader::DoLoadAsynchronously(WebURLRequest request,
                                          WebURLLoaderClient* client) {
  WebURLResponse response;
  
  SDK::ResourceRequest resReq(request.Url().GetString().Ascii());
  SDK::ResourceResponse resResp = backend->ProcessRequest(resReq);

  blink::WebString mimeType =
      blink::WebString::FromASCII(resResp.getMimeType());

  response.SetMimeType(mimeType);
  response.SetHttpStatusCode(200);

  client->DidReceiveResponse(response);
  auto& data = resResp.getData();
  client->DidReceiveData((const char*)&data[0],
                         (int)data.size());
  client->DidFinishLoading(base::TimeTicks::Now(), (int)data.size(),
                           (int)data.size(), (int)data.size(), false);
}

void MyWebURLLoader::LoadAsynchronously(const WebURLRequest& request,
                                        WebURLLoaderClient* client) {
    // Saving the request
    WebURLRequest req = request;
  
    // Running the task for request processing
    PostCrossThreadTask(*GetTaskRunner(), FROM_HERE,
                        CrossThreadBindOnce(&MyWebURLLoader::DoLoadAsynchronously,
                                            CrossThreadUnretained(this),
                                            WTF::Passed(std::move(req)),
                                            CrossThreadUnretained(client)));
}

void MyWebURLLoader::SetDefersLoading(bool defers) {}
void MyWebURLLoader::DidChangePriority(WebURLRequest::Priority, int) {}

scoped_refptr<base::SingleThreadTaskRunner> MyWebURLLoader::GetTaskRunner() {
  return my_web_thread_sched->DefaultTaskRunner();
}

MyWebURLLoaderFactory::MyWebURLLoaderFactory(
    std::shared_ptr<blink::scheduler::WebThreadScheduler> my_web_thread_sched,
    std::shared_ptr<SDK::Backend> backend)
    : my_web_thread_sched(my_web_thread_sched), backend(backend) {}

MyWebURLLoaderFactory::~MyWebURLLoaderFactory() {}

std::unique_ptr<WebURLLoader> MyWebURLLoaderFactory::CreateURLLoader(
    const WebURLRequest&,
    std::unique_ptr<scheduler::WebResourceLoadingTaskRunnerHandle>) {
  return std::make_unique<MyWebURLLoader>(my_web_thread_sched, backend);
}

}  // namespace my_frame_test_helpers
}  // namespace blink
