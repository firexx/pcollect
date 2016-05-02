﻿#include "CurlRequest.h"
#include "CurlUtils.h"

#include <utility>

CurlRequest::CurlRequest() : 
    curl_(newContainer(), &deleteContainer) {
   auto curl = curl_->handle;
    if (curl) { 
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_->error);
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");
    } 
}

CurlContainer* CurlRequest::newContainer() {
    CurlContainer *container = new CurlContainer();
    container->handle = curl_easy_init();
    return container;
}

void CurlRequest::deleteContainer(CurlContainer *container) {
    curl_slist_free_all(container->chunk);
    curl_easy_cleanup(container->handle);
    delete container;
}

void CurlRequest::setHeader(const CurlHeader &header) {
    auto curl = curl_->handle;
    if (curl) { 
        struct curl_slist *chunk = nullptr;
        for (const auto &e : header) { 
            chunk = curl_slist_append(chunk, (e.first + ": " + e.second).c_str());
        } 
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
        curl_->chunk = chunk;
    } 
}

void CurlRequest::setData(const CurlData &data) {
    auto curl = curl_->handle;
    if (curl) { 
        data.content = mergeParams(data.data);
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 0L);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.content.c_str());
    } 
}

void CurlRequest::setGet() {
    auto curl = curl_->handle;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        curl_easy_setopt(curl, CURLOPT_POST, 0L);
    }
}

CurlResponse CurlRequest::makeRequest(const CurlUrl &url, const CurlParams &params) {
    auto curl = curl_->handle;
    if (!params.empty()) {
        curl_easy_setopt(curl, CURLOPT_URL, (url + "?" + mergeParams(params)).c_str());
    } else {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    }

    curl_->error[0] = '\0';

    std::stringstream response_text;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeStream<std::stringstream>);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_text);

    //curl_easy_setopt(curl, CURLOPT_PROXY, "127.0.0.1:8888");
    //curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    //curl_easy_setopt(curl, CURLOPT_CAPATH, "C:\\Users\\fxx\\Desktop\\FiddlerRoot.cer");

    curl_easy_perform(curl);

    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    CurlCookies response_cookies;
    struct curl_slist *p_cookies;
    curl_easy_getinfo(curl, CURLINFO_COOKIELIST, &p_cookies);
    struct curl_slist *temp_cookies = p_cookies;
    while (temp_cookies) {
        auto elements = splitString(temp_cookies->data, '\t');
        auto index = elements.size() - 1;
        response_cookies[elements[index - 1]] = elements[index]; 

        temp_cookies = temp_cookies->next;
    }
    curl_slist_free_all(p_cookies);
    
    return CurlResponse(std::move(response_code), response_text.str(), std::move(response_cookies));
}

CurlResponse CurlRequest::CurlGet(CurlUrl url, CurlHeader header, CurlParams params) {
    setHeader(header);
    setGet();
	return makeRequest(url, params); 
}

CurlResponse CurlRequest::CurlPost(CurlUrl url, CurlData data, CurlHeader header, CurlParams params) {
    setHeader(header);
    setData(data);
	return makeRequest(url, params); 
}
