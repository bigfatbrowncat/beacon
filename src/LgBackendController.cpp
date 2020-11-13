#include "LgBackendController.h"

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
    "                        <label for=\"cars\">Choose a car:</label>"
    "                        <select name=\"cars\" id=\"cars\">"
    "                            <option value=\"volvo\">Volvo</option>"
    "                            <option value=\"saab\">Saab</option>"
    "                            <option value=\"opel\">Opel</option>"
    "                            <option value=\"audi\">Audi</option>"
    "                        </select>"
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

        //    "::selection { background: rgba(0, 0, 128, 0.2); } "

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
        "}";

    std::vector<uint8_t> data(htmlText.begin(), htmlText.end());
    response.setData(data);
  }

  return response;
}

Backend::Backend() { }
Backend::~Backend() { }

}  // namespace SDK
