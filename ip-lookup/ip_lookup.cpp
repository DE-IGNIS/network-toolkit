#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <string>

#pragma comment(lib, "winhttp.lib")

std::string sendGETRequest(const std::wstring &server, const std::wstring &path)
{
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    std::string response;

    // Initialize WinHTTP session
    hSession = WinHttpOpen(
        L"CPP HTTP Client/1.0",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    if (hSession)
    {
        // Connect to server
        hConnect = WinHttpConnect(
            hSession,
            server.c_str(),
            INTERNET_DEFAULT_HTTP_PORT,
            0);
    }

    if (hConnect)
    {
        // Open request
        hRequest = WinHttpOpenRequest(
            hConnect,
            L"GET",
            path.c_str(),
            NULL,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            0);
    }

    if (hRequest)
    {
        // Send request
        BOOL bResults = WinHttpSendRequest(
            hRequest,
            WINHTTP_NO_ADDITIONAL_HEADERS,
            0,
            WINHTTP_NO_REQUEST_DATA,
            0,
            0,
            0);

        if (bResults)
        {
            // Receive response
            bResults = WinHttpReceiveResponse(hRequest, NULL);
        }

        if (bResults)
        {
            DWORD dwSize = 0;
            DWORD dwDownloaded = 0;

            do
            {
                // Check available data
                dwSize = 0;
                WinHttpQueryDataAvailable(hRequest, &dwSize);

                if (dwSize > 0)
                {
                    // Allocate buffer
                    char *buffer = new char[dwSize + 1];
                    ZeroMemory(buffer, dwSize + 1);

                    // Read data
                    WinHttpReadData(
                        hRequest,
                        buffer,
                        dwSize,
                        &dwDownloaded);

                    // Append to response
                    response.append(buffer, dwDownloaded);

                    delete[] buffer;
                }
            } while (dwSize > 0);
        }
    }

    // Cleanup
    if (hRequest)
        WinHttpCloseHandle(hRequest);
    if (hConnect)
        WinHttpCloseHandle(hConnect);
    if (hSession)
        WinHttpCloseHandle(hSession);

    return response;
}

std::string extractValue(const std::string &json, const std::string &key)
{
    std::string searchKey = "\"" + key + "\"";
    size_t keyPos = json.find(searchKey);

    if (keyPos == std::string::npos)
        return "Not Found";

    size_t colonPos = json.find(":", keyPos);

    size_t valueStart = json.find_first_not_of(" ", colonPos + 1);

    std::string value;

    if (json[valueStart] == '"')
    {
        size_t valueEnd = json.find("\"", valueStart + 1);
        value = json.substr(valueStart + 1, valueEnd - valueStart - 1);
    }
    else
    {
        size_t valueEnd = json.find_first_of(",}", valueStart);
        value = json.substr(valueStart, valueEnd - valueStart);
    }

    return value;
}

void format_json(const std::string &res)
{
    std::string country = extractValue(res, "country");
    std::string regionName = extractValue(res, "regionName");
    std::string zip = extractValue(res, "zip");
    std::string city = extractValue(res, "city");
    std::string isp = extractValue(res, "isp");
    std::string ip = extractValue(res, "query");

    std::cout << "========================================" << std::endl;
    std::cout << "  IP          :  " << ip << std::endl;
    std::cout << "  Country     :  " << country << std::endl;
    std::cout << "  Region      :  " << regionName << std::endl;
    std::cout << "  City        :  " << city << std::endl;
    std::cout << "  ZIP Code    :  " << zip << std::endl;
    std::cout << "  ISP         :  " << isp << std::endl;
    std::cout << "========================================" << std::endl;
}

int main()
{
    std::wstring look_ip;

    std::cout << "Enter the ip you want to lookup: ";
    std::getline(std::wcin, look_ip);

    std::cout << "\n";

    std::cout << "Fetching IP geolocation data..." << std::endl;
    std::cout << "========================================" << std::endl;

    // URL: https://ip-api.com/#2405:201:c04c:2898:fad0:7d70:84de:9fe5
    // Server: ip-api.com
    // Path: /json/2405:201:c04c:2898:fad0:7d70:84de:9fe5

    std::wstring server = L"ip-api.com";
    std::wstring path = L"/json/" + look_ip;

    std::string jsonResponse = sendGETRequest(server, path);

    if (!jsonResponse.empty())
    {
        format_json(jsonResponse);
        std::cout << "\n";
    }
    else
    {
        std::cerr << "Failed to get response" << std::endl;
    }

    return 0;
}