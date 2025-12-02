#include <eosio/net_plugin/address_manager.hpp>
#include <eosio/net_plugin/net_plugin.hpp>


namespace eosio {

   void address_manager::add_address(const peer_address &address) {
      std::lock_guard <std::mutex> lock(addresses_mutex);
      dlog("Address Manager add_address: ${host} ${port} ${type}",
              ("host", address.host)("port", address.port)("type", address_type_str(address.address_type)));
      auto it = std::find(addresses.begin(), addresses.end(), address);
      if (it != addresses.end()) {
         return;
      }
      addresses.emplace_back(address);
   }

   void address_manager::add_or_update_address(const peer_address &address) {
      peer_address pa = address;
      std::lock_guard <std::mutex> lock(addresses_mutex);
      dlog("Address Manager add_or_update_address: ${host} ${port} ${type}",
              ("host", address.host)("port", address.port)("type", address_type_str(address.address_type)));
      auto it = std::find(addresses.begin(), addresses.end(), address);
      if (it != addresses.end()) {
         it->manual = pa.manual;
         it->receive = pa.receive;
      } else {
         addresses.emplace_back(pa);
      }
   }

   void address_manager::touch_address(const std::string &address) {
      peer_address pa = peer_address::from_str(address);
      pa.last_active = fc::time_point::now();
      add_or_update_address(pa);
   }

   void address_manager::add_address_str(const std::string &address, bool is_manual) {
      peer_address addr = peer_address::from_str(address, is_manual);
      // same address with different configurations is ignored
      add_address(addr);
   }

   void address_manager::add_active_address(const std::string &address) {
      peer_address addr = peer_address::from_str(address, false);
      addr.last_active = fc::time_point::now();
      add_address(addr);
   }

   void address_manager::add_addresses(const std::unordered_set <std::string> &new_addresses_str, bool is_manual) {
      std::lock_guard <std::mutex> lock(addresses_mutex);
      for (const auto &address: new_addresses_str) {
         peer_address pa = peer_address::from_str(address);
         // Check if address already exists in the map
         if (std::find(addresses.begin(), addresses.end(), pa) == addresses.end()) {
            pa.manual = is_manual;
            addresses.emplace_back(pa);
         }
      }
   }

   void address_manager::remove_address(const peer_address &address) {
      std::lock_guard <std::mutex> lock(addresses_mutex);
      auto iter = std::find(addresses.begin(), addresses.end(), address);
      if (iter != addresses.end()) {
         addresses.erase(iter);
      }
   }

   void address_manager::remove_address_str(const std::string &address) {
      remove_address(peer_address::from_str(address));
   }

   void address_manager::remove_addresses_str(const std::unordered_set <string> &addresses_to_remove) {
      std::lock_guard <std::mutex> lock(addresses_mutex);
      for (const auto &address_str: addresses_to_remove) {
         peer_address pa = peer_address::from_str(address_str);
         auto it = std::find(addresses.begin(), addresses.end(), pa);
         if (it != addresses.end()) {
            addresses.erase(it);
            dlog("Address Manager remove_address: ${host}", ("host", it->host));
         }
      }
   }

   void address_manager::update_address(const peer_address &updated_address) {
      std::lock_guard <std::mutex> lock(addresses_mutex);
      auto it = std::find(addresses.begin(), addresses.end(), updated_address);
      if (it != addresses.end()) {
         *it = updated_address;
      }
   }

   std::unordered_set <std::string> address_manager::get_addresses() const {
      std::lock_guard <std::mutex> lock(addresses_mutex);
      std::unordered_set <std::string> result;
      result.reserve(addresses.size());
      for (const auto &item: addresses) {
         result.emplace(item.to_str());
      }
      return result;
   }

   peer_address address_manager::get_addresses_map(const std::string &address) const {
      std::lock_guard<std::mutex> lock(addresses_mutex);
      peer_address pa = peer_address::from_str(address);
      auto it = std::find(addresses.begin(), addresses.end(), pa);
      if (it != addresses.end()) {
         return *it;
      }
      return peer_address();
   }

   std::unordered_set <std::string> address_manager::get_manual_addresses() const {
      std::lock_guard <std::mutex> lock(addresses_mutex);
      std::unordered_set <std::string> manual_addresses;
      for (const auto &address: addresses) {
         if (address.manual) {
            manual_addresses.insert(address.to_str());
         }
      }
      return manual_addresses;
   }

   std::unordered_set <string>
   address_manager::get_diff_addresses(const std::unordered_set <string> &addresses_exist, bool manual) const {
      std::unordered_set <string> diff_addresses;
      std::unordered_set <string> addr_str_set = manual ? get_manual_addresses() : get_addresses();
      for (const auto &addr_str: addr_str_set) {
         if (addresses_exist.find(addr_str) == addresses_exist.end()) {
            diff_addresses.insert(addr_str);
         }
      }
      return diff_addresses;
   }

   std::unordered_set <std::string>
   address_manager::get_latest_active_addresses(const std::chrono::seconds &secs, bool manual) const {
      std::lock_guard <std::mutex> lock(addresses_mutex);

      std::unordered_set <std::string> active_addresses;
      fc::time_point oldest_time = fc::time_point::now() - fc::seconds(secs.count());

      for (const auto &address: addresses) {
         if ((!manual || address.manual == manual) && address.last_active >= oldest_time) {
            active_addresses.insert(address.to_str());
         }
      }
      return active_addresses;
   }


   bool address_manager::has_address(const std::string &address_str) const {
      std::lock_guard <std::mutex> lock(addresses_mutex);
      peer_address pa = peer_address::from_str(address_str);
      auto it = std::find(addresses.begin(), addresses.end(), pa);
      return it != addresses.end();
   }

   uint32_t address_manager::get_addresses_count() const {
      std::lock_guard <std::mutex> lock(addresses_mutex);
      return addresses.size();
   }
}
