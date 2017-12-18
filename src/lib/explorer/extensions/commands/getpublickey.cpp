/**
 * Copyright (c) 2016-2017 mvs developers 
 *
 * This file is part of metaverse-explorer.
 *
 * metaverse-explorer is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include <metaverse/explorer/dispatch.hpp>
#include <metaverse/explorer/extensions/commands/getpublickey.hpp>
#include <metaverse/explorer/extensions/command_extension_func.hpp>
#include <metaverse/explorer/extensions/command_assistant.hpp>
#include <metaverse/explorer/extensions/exception.hpp>

namespace libbitcoin {
namespace explorer {
namespace commands {
using namespace bc::explorer::config;

console_result getpublickey::invoke (std::ostream& output,
        std::ostream& cerr, libbitcoin::server::server_node& node)
{
    auto& blockchain = node.chain_impl();
    blockchain.is_account_passwd_valid(auth_.name, auth_.auth);
    if (!argument_.address.empty() && !blockchain.is_valid_address(argument_.address))
        throw address_invalid_exception{"invalid address parameter!"};

    auto addr = bc::wallet::payment_address(argument_.address);
    if(addr.version() == 0x05) // for multisig address
        throw argument_legality_exception{"script address parameter not allowed!"};
    
    auto pvaddr = blockchain.get_account_addresses(auth_.name);
    if(!pvaddr) 
        throw address_list_nullptr_exception{"nullptr for address list"};
    
    // set random address
    if (argument_.address.empty()) {
        auto random = bc::pseudo_random();
        auto index = random % pvaddr->size();
        argument_.address = pvaddr->at(index).get_address();
    }

    const char* cmds[2]{"ec-to-public", nullptr};
    std::ostringstream sout("account not have the address!");
    std::istringstream sin; 
    std::string prv_key;
    // get public key
    auto found = false;
    for (auto& each : *pvaddr){
        if(each.get_address() == argument_.address) {
            prv_key = each.get_prv_key(auth_.auth);
            cmds[1] = prv_key.c_str();
            if(console_result::okay == dispatch_command(2, cmds, sin, sout, sout))
                found = true;
            break;
        }
    }

    if(!found) throw account_address_get_exception{sout.str()};
    
    Json::Value root;
    root["public-key"] = sout.str();
    root["address"] = argument_.address;
    output << root.toStyledString();
    
    return console_result::okay;
}


} // namespace commands
} // namespace explorer
} // namespace libbitcoin

