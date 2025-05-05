#include "sendrecv.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <vector>

ssize_t my_send(const int& fd, const std::string& data)
{
    uint32_t netlen = htonl(static_cast<uint32_t>(data.size()));

    ssize_t ret = send(fd, &netlen, sizeof(netlen), MSG_NOSIGNAL);
    if (ret != sizeof(netlen)) {
        return (ret < 0 ? ret : -1);
    }

    size_t total = 0;
    while (total < data.size()) {
        ret = send(fd, data.data() + total, data.size() - total, MSG_NOSIGNAL);
        if (ret <= 0) {
            return ret;
        }
        total += ret;
    }

    return static_cast<ssize_t>(total);
}

ssize_t my_recv(const int& fd, std::string& data)
{
    uint32_t netlen;
    ssize_t ret = recv(fd, &netlen, sizeof(netlen), MSG_WAITALL);
    if (ret <= 0)
        return ret;
    if (ret != sizeof(netlen))
        return -1;
    uint32_t message_size = ntohl(netlen);

    std::vector<char> buffer(message_size);
    ssize_t total = 0;
    while (total < (ssize_t)message_size) {
        ret = recv(fd, buffer.data() + total, message_size - total, 0);
        if (ret <= 0)
            return ret;
        total += ret;
    }

    data.assign(buffer.data(), message_size);
    return total;
}

std::vector<char> load_file_bytes(const std::string& path)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        throw std::runtime_error("Can't open file: " + path);
    }

    std::vector<char> buffer{std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>()};
    return buffer;
}

void save_file_bytes(const std::string& path, const std::vector<char>& buffer)
{
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) {
        throw std::runtime_error("Can't open file for writing: " + path);
    }

    std::copy(buffer.begin(), buffer.end(), std::ostreambuf_iterator<char>(ofs));
}

ssize_t send_file(const int& fd, const std::string& filename)
{
    std::vector<char> raw_data = load_file_bytes(filename);

    ssize_t sended = 0;
    ssize_t message_size = htonl(raw_data.size());

    ssize_t send_on_this_interation = send(fd, &message_size, sizeof(message_size), 0);

    while (sended < raw_data.size()) {
        if (send_on_this_interation < 0)
            return send_on_this_interation;
        send_on_this_interation = send(fd, raw_data.data() + sended, raw_data.size() - sended, 0);
        sended += send_on_this_interation;
    }

    return sended;
}

ssize_t recv_file(const int& fd, const std::string& filename)
{
    ssize_t received = 0;
    ssize_t message_size;
    ssize_t received_on_this_interation = recv(fd, &message_size, sizeof(message_size), 0);
    if (received_on_this_interation < 0)
        return received_on_this_interation;
    message_size = ntohl(message_size);

    std::vector<char> raw_data;
    raw_data.resize(message_size);

    while (received < message_size) {
        received_on_this_interation = recv(fd, raw_data.data() + received, message_size - received, 0);
        if (received_on_this_interation < 0) {
            return received_on_this_interation;
        }
        received += received_on_this_interation;
    }

    save_file_bytes(filename, raw_data);
    return received;
}