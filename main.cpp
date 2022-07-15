#include <iostream>
#include <span>
#include <thread>

#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/session.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/torrent_status.hpp>

#include <fmt/format.h>

namespace {
volatile std::sig_atomic_t g_isOpen = true;
}

void signal_handler(int signal) {

  if (signal == SIGINT) {
    g_isOpen = false;
  }
}

struct TorrentInfo {
  TorrentInfo() {}
  TorrentInfo(lt::torrent_handle &handle) {
    auto status = handle.status();

    name = status.name;
    progress = status.progress;

    state = status.state;
  }

  std::string getStringTorrentStatus() const {
    switch (state) {
    case lt::torrent_status::checking_files:
      return "checking";
    case lt::torrent_status::downloading_metadata:
      return "dl metadata";
    case lt::torrent_status::downloading:
      return "downloading";
    case lt::torrent_status::finished:
      return "finished";
    case lt::torrent_status::seeding:
      return "seeding";
    case lt::torrent_status::checking_resume_data:
      return "checking resume";
    default:
      return "<>";
    }
  }

  std::string name;
  float progress;
  lt::torrent_status::state_t state;
};

class DownloadTorrentStatus {
public:
  DownloadTorrentStatus(std::vector<lt::torrent_handle> handleList)
      : _handleList(handleList) {}

  std::vector<TorrentInfo> getTorrentsInfo() {
    std::vector<TorrentInfo> infoList;

    std::transform(_handleList.begin(), _handleList.end(),
                   std::insert_iterator(infoList, infoList.end()),
                   [](auto &handle) { return TorrentInfo{handle}; });

    return infoList;
  }

  std::size_t torrentCount() const { return _handleList.size(); }

private:
  std::vector<lt::torrent_handle> _handleList;
};

class Session {
public:
  void addTorrentFromFile(const std::string &fileName) {

    lt::add_torrent_params atp;
    atp.save_path = ".";
    atp.ti = std::make_shared<lt::torrent_info>(fileName);
    _session.add_torrent(atp);
  }

  void addTorrentFromMagnet(const std::string &uri) {

    lt::add_torrent_params atp = lt::parse_magnet_uri(uri);
    atp.save_path = ".";
    _session.add_torrent(atp);
  }

  void addTorrent(const std::string &string) {

    if (string.starts_with("magnet:")) {
      addTorrentFromMagnet(string);
    } else {
      addTorrentFromFile(string);
    }
  }

  DownloadTorrentStatus getDownloadTorrentStatus() {
    return _session.get_torrents();
  }

private:
  lt::session _session;
};

void printTorrentInfo(const TorrentInfo &info) {
  const auto status = info.getStringTorrentStatus();
  const auto name = info.name;
  const auto progress = info.progress * 100.0f;

  fmt::print("[{}] {}  {:.2f}%\n", status, name, progress);
}

void mainLoop(Session &session) {
  auto downloadTorrentStatus = session.getDownloadTorrentStatus();

  if (downloadTorrentStatus.torrentCount() == 0) {
    return;
  }

  while (g_isOpen) {
    const auto &torrentsInfo = downloadTorrentStatus.getTorrentsInfo();

    std::for_each(torrentsInfo.begin(), torrentsInfo.end(),
                  [](const auto &info) { printTorrentInfo(info); });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
}

int main(int argc, char **argv) {

  std::signal(SIGINT, signal_handler);

  const auto arguments = std::span(argv, argc);

  Session session;

  std::for_each(arguments.begin() + 1, arguments.end(),
                [&session](auto string) { session.addTorrent(string); });

  mainLoop(session);

  return EXIT_SUCCESS;
}
