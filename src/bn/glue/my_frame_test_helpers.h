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

#ifndef MY_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_FRAME_TEST_HELPERS_H_
#define MY_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_FRAME_TEST_HELPERS_H_

#define COMPILE_CONTENT_STATICALLY 1

#include <iostream>
#include <memory>
#include <string>

#include "base/location.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/sequence_local_storage_map.h"
#include "cc/test/test_task_graph_runner.h"
#include "cc/trees/layer_tree_host.h"
#include "cc/trees/ukm_manager.h"
#include "components/ukm/ukm_recorder_impl.h"
#include "content/common/content_export.h"
#include "content/renderer/compositor/layer_tree_view.h"
#include "content/renderer/compositor/layer_tree_view_delegate.h"
#include "content/test/stub_layer_tree_view_delegate.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/common/frame/frame_owner_element_type.h"
#include "third_party/blink/public/common/input/web_keyboard_event.h"
#include "third_party/blink/public/common/input/web_mouse_event.h"
#include "third_party/blink/public/common/input/web_mouse_wheel_event.h"
#include "third_party/blink/public/mojom/fetch/fetch_api_request.mojom-blink-forward.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/scheduler/test/web_fake_thread_scheduler.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url_loader_client.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/public/web/web_frame_owner_properties.h"
#include "third_party/blink/public/web/web_history_item.h"
#include "third_party/blink/public/web/web_local_frame_client.h"
#include "third_party/blink/public/web/web_remote_frame_client.h"
#include "third_party/blink/public/web/web_settings.h"
#include "third_party/blink/public/web/web_view_client.h"
#include "third_party/blink/renderer/core/exported/web_view_impl.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/scroll/scrollbar_theme.h"
#include "third_party/blink/renderer/core/testing/scoped_mock_overlay_scrollbars.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/wtf/allocator/allocator.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

#include <bn/sdk/BNBackendController.h>
#include <bn/impl/BNLayer.h>

namespace beacon::impl {
  class BNLayer;
}

namespace base {
class TickClock;
}

namespace cc {
class AnimationHost;
}

namespace blink {
class WebFrame;
class WebLocalFrameImpl;
struct WebNavigationParams;
class WebRemoteFrameImpl;
class WebSettings;
}  // namespace blink

namespace beacon::glue {

using namespace blink;

class TestWebFrameClient;
class TestWebRemoteFrameClient;
class TestWebWidgetClient;
class TestWebViewClient;
class WebViewHelper;

// Loads a url into the specified WebLocalFrame for testing purposes.
void LoadFrameDontWait(WebLocalFrame*, const WebURL& url);
// Same as above, but also pumps any pending resource requests,
// as well as waiting for the threaded parser to finish, before returning.
void LoadFrame(WebLocalFrame*, const std::string& url);
// Same as above, but for WebLocalFrame::LoadHTMLString().
void LoadHTMLString(WebLocalFrame*,
                    const std::string& html,
                    const WebURL& base_url,
                    const base::TickClock* clock = nullptr);
// Same as above, but for WebLocalFrame::RequestFromHistoryItem/Load.
void LoadHistoryItem(WebLocalFrame*,
                     const WebHistoryItem&,
                     mojom::FetchCacheMode);
// Same as above, but for WebLocalFrame::Reload().
void ReloadFrame(WebLocalFrame*);
void ReloadFrameBypassingCache(WebLocalFrame*);

// Fills navigation params if needed. Params should have the proper url set up.
void FillNavigationParamsResponse(WebNavigationParams* params,
                                  beacon::sdk::Backend* backend);

// Pumps pending resource requests while waiting for a frame to load. Consider
// using one of the above helper methods whenever possible.
void PumpPendingRequestsForFrameToLoad(WebLocalFrame*);

WebMouseEvent CreateMouseEvent(WebInputEvent::Type type,
                               WebMouseEvent::Button button,
                               const IntPoint& point,
                               int modifiers);

WebMouseWheelEvent CreateMouseWheelEvent(WebMouseWheelEvent::Phase phase,
                                         int delta_x,
                                         int delta_y,
                                         int modifiers);

WebKeyboardEvent CreateKeyboardEvent(char key_code,
                                     int modifiers,
                                     WebInputEvent::Type type);

// Helpers for creating frames for test purposes. All methods that accept raw
// pointer client arguments allow nullptr as a valid argument; if a client
// pointer is null, the test framework will automatically create and manage the
// lifetime of that client interface. Otherwise, the caller is responsible for
// ensuring that non-null clients outlive the created frame.

// Helper for creating a local child frame of a local parent frame.
WebLocalFrameImpl* CreateLocalChild(WebLocalFrame& parent,
                                    WebTreeScopeType,
                                    TestWebFrameClient* = nullptr);

// Similar, but unlike the overload which takes the client as a raw pointer,
// ownership of the TestWebFrameClient is transferred to the test framework.
// TestWebFrameClient may not be null.
WebLocalFrameImpl* CreateLocalChild(WebLocalFrame& parent,
                                    WebTreeScopeType,
                                    std::unique_ptr<TestWebFrameClient>);

// Helper for creating a provisional local frame that can replace a remote
// frame.
WebLocalFrameImpl* CreateProvisional(WebRemoteFrame& old_frame,
                                     TestWebFrameClient* = nullptr);

// Helper for creating a remote frame. Generally used when creating a remote
// frame to swap into the frame tree.
// TODO(dcheng): Consider allowing security origin to be passed here once the
// frame tree moves back to core.
WebRemoteFrameImpl* CreateRemote(TestWebRemoteFrameClient* = nullptr);

// Helper for creating a local child frame of a remote parent frame.
WebLocalFrameImpl* CreateLocalChild(
    WebRemoteFrame& parent,
    const WebString& name = WebString(),
    const WebFrameOwnerProperties& = WebFrameOwnerProperties(),
    WebFrame* previous_sibling = nullptr,
    TestWebFrameClient* = nullptr,
    TestWebWidgetClient* = nullptr);

// Helper for creating a remote child frame of a remote parent frame.
WebRemoteFrameImpl* CreateRemoteChild(WebRemoteFrame& parent,
                                      const WebString& name = WebString(),
                                      scoped_refptr<SecurityOrigin> = nullptr,
                                      TestWebRemoteFrameClient* = nullptr);

class TestUkmRecorderFactory : public cc::UkmRecorderFactory {
 public:
  ~TestUkmRecorderFactory() override;

  std::unique_ptr<ukm::UkmRecorder> CreateRecorder() override;
};

class StubLayerTreeViewDelegate : public content::LayerTreeViewDelegate {
 public:
  // LayerTreeViewDelegate implementation.
  void ApplyViewportChanges(const cc::ApplyViewportChangesArgs&) override {}
  void RecordManipulationTypeCounts(cc::ManipulationInfo info) override {}
  void SendOverscrollEventFromImplSide(
      const gfx::Vector2dF& overscroll_delta,
      cc::ElementId scroll_latched_element_id) override {}
  void SendScrollEndEventFromImplSide(
      cc::ElementId scroll_latched_element_id) override {}
  void BeginMainFrame(base::TimeTicks frame_time) override {}
  void OnDeferMainFrameUpdatesChanged(bool) override {}
  void OnDeferCommitsChanged(bool) override {}
  void DidBeginMainFrame() override {}
  void RecordStartOfFrameMetrics() override {}
  void RecordEndOfFrameMetrics(base::TimeTicks) override {}
  std::unique_ptr<cc::BeginMainFrameMetrics> GetBeginMainFrameMetrics()
      override;
  void BeginUpdateLayers() override {}
  void EndUpdateLayers() override {}
  void RequestNewLayerTreeFrameSink(
      LayerTreeFrameSinkCallback callback) override;
  void DidCommitAndDrawCompositorFrame() override {
    std::cout << "DidCommitAndDrawCompositorFrame()" << std::endl;
  }
  void WillCommitCompositorFrame() override {
    std::cout << "WillCommitCompositorFrame()" << std::endl;
  }
  void DidCommitCompositorFrame() override {
    std::cout << "DidCommitCompositorFrame()" << std::endl;
  }
  void DidCompletePageScaleAnimation() override {}
  void UpdateVisualState() override {}
  void WillBeginCompositorFrame() override {}
};

struct InjectedScrollGestureData {
  WebFloatSize delta;
  ScrollGranularity granularity;
  CompositorElementId scrollable_area_element_id;
  WebInputEvent::Type type;
};

class TestWebWidgetClient : public WebWidgetClient {
 public:
  // If no delegate is given, a stub is used.
  explicit TestWebWidgetClient(
      content::LayerTreeViewDelegate* delegate = nullptr,
      scoped_refptr<base::SingleThreadTaskRunner> mainTaskRunner = nullptr,
      scoped_refptr<base::SingleThreadTaskRunner> composeTaskRunner = nullptr,
      scheduler::WebThreadScheduler* my_web_thread_sched = nullptr);

  ~TestWebWidgetClient() override;

  // WebWidgetClient implementation.
  void ScheduleAnimation() override;
  void SetRootLayer(scoped_refptr<cc::Layer> layer) override;
  void RegisterSelection(const cc::LayerSelection& selection) override;
  void SetBackgroundColor(SkColor color) override;
  void SetPageScaleStateAndLimits(float page_scale_factor,
                                  bool is_pinch_gesture_active,
                                  float minimum,
                                  float maximum) override;
  void InjectGestureScrollEvent(WebGestureDevice device,
                                const WebFloatSize& delta,
                                ScrollGranularity granularity,
                                cc::ElementId scrollable_area_element_id,
                                WebInputEvent::Type injected_type) override;
  void SetHaveScrollEventHandlers(bool) override;
  void SetEventListenerProperties(
      cc::EventListenerClass event_class,
      cc::EventListenerProperties properties) override;
  cc::EventListenerProperties EventListenerProperties(
      cc::EventListenerClass event_class) const override;
  std::unique_ptr<cc::ScopedDeferMainFrameUpdate> DeferMainFrameUpdate()
      override;
  void StartDeferringCommits(base::TimeDelta timeout) override;
  void StopDeferringCommits(cc::PaintHoldingCommitTrigger) override;
  void DidMeaningfulLayout(WebMeaningfulLayout) override;
  
  // Called to show the widget according to the given policy.
  void Show(WebNavigationPolicy) override {
    std::cout << "TestWebWidgetClient::Show()" << std::endl;
  }

  void SetBrowserControlsShownRatio(float top_ratio,
                                    float bottom_ratio) override;
  void SetBrowserControlsParams(cc::BrowserControlsParams) override;
  viz::FrameSinkId GetFrameSinkId() override;

  // This function is called to send the collected scroll events to a WebWidget
  void HandleScrollEvents(WebWidget* widget);

  cc::LayerTreeHost* layer_tree_host() {
    return layer_tree_view_->layer_tree_host();
  }
  const cc::LayerTreeHost* layer_tree_host() const {
    return layer_tree_view_->layer_tree_host();
  }
  cc::AnimationHost* animation_host() { return animation_host_; }

  bool AnimationScheduled() { return animation_scheduled_; }
  void ClearAnimationScheduled() { animation_scheduled_ = false; }

  // Returns the last value given to SetHaveScrollEventHandlers().
  bool HaveScrollEventHandlers() const { return have_scroll_event_handlers_; }

  int VisuallyNonEmptyLayoutCount() const {
    return visually_non_empty_layout_count_;
  }
  int FinishedParsingLayoutCount() const {
    return finished_parsing_layout_count_;
  }
  int FinishedLoadingLayoutCount() const {
    return finished_loading_layout_count_;
  }

 private:
  std::unique_ptr<content::LayerTreeView> layer_tree_view_ = nullptr;       // TODO: replace with LayerTreeHostImpl
  cc::AnimationHost* animation_host_ = nullptr;
  // LayerTreeViewFactory layer_tree_view_factory_;
  Vector<InjectedScrollGestureData> injected_scroll_gesture_data_;
  bool animation_scheduled_ = false;
  bool have_scroll_event_handlers_ = false;
  int visually_non_empty_layout_count_ = 0;
  int finished_parsing_layout_count_ = 0;
  int finished_loading_layout_count_ = 0;
};

class TestWebViewClient : public WebViewClient {
 public:
  TestWebViewClient(std::shared_ptr<WebViewHelper> parent);
  ~TestWebViewClient() override;

  void DestroyChildViews();

  // WebViewClient overrides.
  bool CanHandleGestureEvent() override;
  bool CanUpdateLayout() override;
  WebView* CreateView(WebLocalFrame* opener,
                      const WebURLRequest&,
                      const WebWindowFeatures&,
                      const WebString& name,
                      WebNavigationPolicy,
                      WebSandboxFlags,
                      const FeaturePolicy::FeatureState&,
                      const SessionStorageNamespaceId&) override;
  void FocusNext() override;
  void FocusPrevious() override;

 private:
  // LayerTreeViewFactory layer_tree_view_factory_;
  WTF::Vector<std::unique_ptr<WebViewHelper>> child_web_views_;
  std::shared_ptr<WebViewHelper> parent;

 public:
  // Called when a region of the WebView needs to be re-painted. This is only
  // for non-composited WebViews that exist to contribute to a "parent" WebView
  // painting. Otherwise invalidations are transmitted to the compositor through
  // the layers.
  void DidInvalidateRect(const WebRect&) override {
    std::cout << "DidInvalidateRect()" << std::endl;
  }
};

// Convenience class for handling the lifetime of a WebView and its associated
// mainframe in tests.
class WebViewHelper /*: public ScopedMockOverlayScrollbars*/ {
  USING_FAST_MALLOC(WebViewHelper);

 public:
  WebViewHelper();
  ~WebViewHelper();

  // Helpers for creating the main frame. All methods that accept raw
  // pointer client arguments allow nullptr as a valid argument; if a client
  // pointer is null, the test framework will automatically create and manage
  // the lifetime of that client interface. Otherwise, the caller is responsible
  // for ensuring that non-null clients outlive the created frame.

  // Creates and initializes the WebView with a main WebLocalFrame.
  WebViewImpl* InitializeWithOpener(
      WebFrame* opener,
      TestWebFrameClient* = nullptr,
      TestWebViewClient* = nullptr,
      TestWebWidgetClient* = nullptr,
      void (*update_settings_func)(WebSettings*) = nullptr);

  // Same as InitializeWithOpener(), but always sets the opener to null.
  WebViewImpl* Initialize(TestWebFrameClient* = nullptr,
                          TestWebViewClient* = nullptr,
                          TestWebWidgetClient* = nullptr,
                          void (*update_settings_func)(WebSettings*) = nullptr);

  // Same as InitializeWithOpener(), but passes null for everything but the
  // settings function.
  WebViewImpl* InitializeWithSettings(
      void (*update_settings_func)(WebSettings*));

  // Same as Initialize() but also performs the initial load of the url. Only
  // returns once the load is complete.
  WebViewImpl* InitializeAndLoad(
      const std::string& url,
      TestWebFrameClient* = nullptr,
      TestWebViewClient* = nullptr,
      TestWebWidgetClient* = nullptr,
      void (*update_settings_func)(WebSettings*) = nullptr);

  // Same as InitializeRemoteWithOpener(), but always sets the opener to null.
  WebViewImpl* InitializeRemote(TestWebRemoteFrameClient* = nullptr,
                                scoped_refptr<SecurityOrigin> = nullptr,
                                TestWebViewClient* = nullptr,
                                TestWebWidgetClient* = nullptr);

  // Creates and initializes the WebView with a main WebRemoteFrame. Passing
  // nullptr as the SecurityOrigin results in a frame with a unique security
  // origin.
  WebViewImpl* InitializeRemoteWithOpener(
      WebFrame* opener,
      TestWebRemoteFrameClient* = nullptr,
      scoped_refptr<SecurityOrigin> = nullptr,
      TestWebViewClient* = nullptr,
      TestWebWidgetClient* = nullptr);

  // Load the 'Ahem' font to this WebView.
  // The 'Ahem' font is the only font whose font metrics is consistent across
  // platforms, but it's not guaranteed to be available.
  // See external/wpt/css/fonts/ahem/README for more about the 'Ahem' font.
  void LoadAhem();

  void Resize(WebSize);

  void Reset();

  void SetFocused();

  WebViewImpl* GetWebView() const { return web_view_; }

  cc::LayerTreeHost* GetLayerTreeHost() const {
    return test_web_widget_client_->layer_tree_host();
  }
  TestWebWidgetClient* GetWebWidgetClient() const {
    return test_web_widget_client_;
  }

  WebLocalFrameImpl* LocalMainFrame() const;
  WebRemoteFrameImpl* RemoteMainFrame() const;

  void set_viewport_enabled(bool viewport) {
    DCHECK(!web_view_)
        << "set_viewport_enabled() should be called before Initialize.";
    viewport_enabled_ = viewport;
  }

  std::unique_ptr<TestWebViewClient> owned_test_web_view_client_;
  TestWebViewClient* test_web_view_client_ = nullptr;
  std::unique_ptr<TestWebWidgetClient> owned_test_web_widget_client_;
  TestWebWidgetClient* test_web_widget_client_ = nullptr;

 private:
  void InitializeWebView(TestWebViewClient*, class WebView* opener);

  bool viewport_enabled_ = false;

  WebViewImpl* web_view_;

  // The Platform should not change during the lifetime of the test!
  Platform* const platform_;

  DISALLOW_COPY_AND_ASSIGN(WebViewHelper);
};

class TestWebFrameClient : public WebLocalFrameClient {
 private:
  std::shared_ptr<scheduler::WebThreadScheduler> my_web_thread_sched;
  std::shared_ptr<beacon::sdk::Backend> backend;
  beacon::impl::BNLayer* window;
 public:
  TestWebFrameClient(std::shared_ptr<beacon::sdk::Backend> backend,
                     beacon::impl::BNLayer* window);
  ~TestWebFrameClient() override;

  void SetScheduler(std::shared_ptr<scheduler::WebThreadScheduler>
                        my_web_thread_sched);

  static bool IsLoading() { return loads_in_progress_ > 0; }
  Vector<String>& ConsoleMessages() { return console_messages_; }

  std::shared_ptr<beacon::sdk::Backend> Backend() { return backend; }

  WebNavigationControl* Frame() const { return frame_; }
  // Pass ownership of the TestWebFrameClient to |self_owned| here if the
  // TestWebFrameClient should delete itself on frame detach.
  void Bind(WebLocalFrame*,
            std::unique_ptr<TestWebFrameClient> self_owned = nullptr);
  // Note: only needed for local roots.
  void BindWidgetClient(std::unique_ptr<WebWidgetClient>);

  // WebLocalFrameClient:
  void FrameDetached(DetachType) override;
  WebLocalFrame* CreateChildFrame(WebLocalFrame* parent,
                                  WebTreeScopeType,
                                  const WebString& name,
                                  const WebString& fallback_name,
                                  const FramePolicy&,
                                  const WebFrameOwnerProperties&,
                                  FrameOwnerElementType) override;
  void DidStartLoading() override;
  void DidStopLoading() override;

  // The frame's document and all of its subresources succeeded to load.
  void DidFinishLoad() override;

  service_manager::InterfaceProvider* GetInterfaceProvider() override;
  std::unique_ptr<WebURLLoaderFactory> CreateURLLoaderFactory() override;
  void BeginNavigation(std::unique_ptr<WebNavigationInfo> info) override;
  WebEffectiveConnectionType GetEffectiveConnectionType() override;
  void SetEffectiveConnectionTypeForTesting(
      WebEffectiveConnectionType) override;
  void DidAddMessageToConsole(const WebConsoleMessage&,
                              const WebString& source_name,
                              unsigned source_line,
                              const WebString& stack_trace) override;
  WebPlugin* CreatePlugin(const WebPluginParams& params) override;
  AssociatedInterfaceProvider* GetRemoteNavigationAssociatedInterfaces()
      override;

  // Called when the frame rects changed.
  void FrameRectsChanged(const WebRect&) override {
    // TODO Redraw here!!!
  }

  virtual void DidChangeContents() override {
    std::cout << "DidChangeContents()" << std::endl;
  }

 private:
  void CommitNavigation(std::unique_ptr<WebNavigationInfo>);

  static int loads_in_progress_;

  // If set to a non-null value, self-deletes on frame detach.
  std::unique_ptr<TestWebFrameClient> self_owned_;

  // Use service_manager::InterfaceProvider::TestApi to provide test interfaces
  // through this client.
  std::unique_ptr<service_manager::InterfaceProvider> interface_provider_;

  std::unique_ptr<AssociatedInterfaceProvider> associated_interface_provider_;

  // This is null from when the client is created until it is initialized with
  // Bind().
  WebNavigationControl* frame_ = nullptr;

  base::CancelableOnceCallback<void()> navigation_callback_;
  std::unique_ptr<WebWidgetClient> owned_widget_client_;
  WebEffectiveConnectionType effective_connection_type_;
  Vector<String> console_messages_;

  base::WeakPtrFactory<TestWebFrameClient> weak_factory_{this};
};

// Minimal implementation of WebRemoteFrameClient needed for unit tests that
// load remote frames. Tests that load frames and need further specialization
// of WebLocalFrameClient behavior should subclass this.
class TestWebRemoteFrameClient : public WebRemoteFrameClient {
 public:
  TestWebRemoteFrameClient();
  ~TestWebRemoteFrameClient() override;

  WebRemoteFrame* Frame() const;
  // Pass ownership of the TestWebFrameClient to |self_owned| here if the
  // TestWebRemoteFrameClient should delete itself on frame detach.
  void Bind(WebRemoteFrame*,
            std::unique_ptr<TestWebRemoteFrameClient> self_owned = nullptr);

  // WebRemoteFrameClient:
  void FrameDetached(DetachType) override;
  void ForwardPostMessage(WebLocalFrame* source_frame,
                          WebRemoteFrame* target_frame,
                          WebSecurityOrigin target_origin,
                          WebDOMMessageEvent) override {}

  AssociatedInterfaceProvider* GetAssociatedInterfaceProvider() {
    return associated_interface_provider_.get();
  }

 private:
  // If set to a non-null value, self-deletes on frame detach.
  std::unique_ptr<TestWebRemoteFrameClient> self_owned_;

  std::unique_ptr<AssociatedInterfaceProvider> associated_interface_provider_;

  // This is null from when the client is created until it is initialized with
  // Bind().
  WebRemoteFrame* frame_ = nullptr;
};

class MyWebURLRequestWrapper {
 private:
  std::shared_ptr<WebURLRequest> request;

 public:
  MyWebURLRequestWrapper(const WebURLRequest& req);
  ~MyWebURLRequestWrapper();
};

class MyWebURLLoader final : public WebURLLoader {
 private:
  std::shared_ptr<scheduler::WebThreadScheduler> my_web_thread_sched;
  std::shared_ptr<beacon::sdk::Backend> backend;

  void DoLoadAsynchronously(WebURLRequest request, WebURLLoaderClient* client);

 public:
  MyWebURLLoader(
      std::shared_ptr<scheduler::WebThreadScheduler> my_web_thread_sched,
      std::shared_ptr<beacon::sdk::Backend> backend);
  ~MyWebURLLoader() override;

  void LoadSynchronously(const WebURLRequest&,
                         WebURLLoaderClient*,
                         WebURLResponse&,
                         base::Optional<WebURLError>&,
                         WebData&,
                         int64_t& encoded_data_length,
                         int64_t& encoded_body_length,
                         WebBlobInfo& downloaded_blob) override;

  void LoadAsynchronously(const WebURLRequest& request,
                          WebURLLoaderClient* client) override;

  void SetDefersLoading(bool defers) override;
  void DidChangePriority(WebURLRequest::Priority, int) override;
  scoped_refptr<base::SingleThreadTaskRunner> GetTaskRunner() override;
};

class MyWebURLLoaderFactory final : public WebURLLoaderFactory {
 private:
  std::shared_ptr<scheduler::WebThreadScheduler> my_web_thread_sched;
  std::shared_ptr<beacon::sdk::Backend> backend;

 public:
  MyWebURLLoaderFactory(
      std::shared_ptr<scheduler::WebThreadScheduler> my_web_thread_sched,
      std::shared_ptr<beacon::sdk::Backend> backend);
  ~MyWebURLLoaderFactory() override;
  std::unique_ptr<WebURLLoader> CreateURLLoader(
      const WebURLRequest&,
      std::unique_ptr<scheduler::WebResourceLoadingTaskRunnerHandle>) override;
};

}  // namespace beacon::glue

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_FRAME_TEST_HELPERS_H_
