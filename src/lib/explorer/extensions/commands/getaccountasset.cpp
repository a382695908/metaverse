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

#include <metaverse/explorer/json_helper.hpp>
#include <metaverse/explorer/dispatch.hpp>
#include <metaverse/explorer/extensions/commands/getaccountasset.hpp>
#include <metaverse/explorer/extensions/command_extension_func.hpp>
#include <metaverse/explorer/extensions/command_assistant.hpp>
#include <metaverse/explorer/extensions/exception.hpp>
#include <metaverse/explorer/extensions/base_helper.hpp>

namespace libbitcoin {
namespace explorer {
namespace commands {
using namespace bc::explorer::config;

/************************ getaccountasset *************************/

console_result getaccountasset::invoke (Json::Value& jv_output,
         libbitcoin::server::server_node& node)
{
    auto& aroot = jv_output;
    Json::Value assets;
    
    std::string symbol;
    auto& blockchain = node.chain_impl();
    
    if (argument_.symbol.length() > ASSET_DETAIL_SYMBOL_FIX_SIZE)
        throw asset_symbol_length_exception{"asset symbol length must be less than 64."};
    
    auto sh_vec = std::make_shared<std::vector<asset_detail>>();
    
    blockchain.is_account_passwd_valid(auth_.name, auth_.auth);
    auto pvaddr = blockchain.get_account_addresses(auth_.name);
    if(!pvaddr) 
        throw address_list_nullptr_exception{"nullptr for address list"};
    
    // 1. get asset in blockchain       
    // get address unspent asset balance
    std::string addr;
    for (auto& each : *pvaddr){
        addr = each.get_address();
        sync_fetch_asset_balance_record (addr, blockchain, sh_vec);
    }

    Json::Value asset_data;
    for (auto& elem: *sh_vec) {
        if(!argument_.symbol.empty() && argument_.symbol !=  elem.get_symbol())
            continue;
        asset_data["symbol"] = elem.get_symbol();
        asset_data["address"] = elem.get_address();
        symbol = elem.get_symbol();
        if (get_api_version() == 1) {
            asset_data["quantity"] += elem.get_maximum_supply();
        } else {
            asset_data["quantity"] = elem.get_maximum_supply();
        }
        //asset_data["address"] = elem.get_address();
        auto issued_asset = blockchain.get_issued_asset(symbol);
        if(issued_asset && get_api_version() == 1) {
            asset_data["decimal_number"] += issued_asset->get_decimal_number();
        }
        if(issued_asset && get_api_version() == 2) {
            asset_data["decimal_number"] = issued_asset->get_decimal_number();
        }
        asset_data["status"] = "unspent";
        assets.append(asset_data);
    }
    // 2. get asset in local database
    // shoudl filter all issued asset which be stored in local account asset database
    sh_vec->clear();
    sh_vec = blockchain.get_issued_assets();
    //std::shared_ptr<std::vector<business_address_asset>>
    auto sh_unissued = blockchain.get_account_unissued_assets(auth_.name);        
    for (auto& elem: *sh_unissued) {
        
        auto symbol = elem.detail.get_symbol();         
        auto pos = std::find_if(sh_vec->begin(), sh_vec->end(), [&](const asset_detail& elem){
            return symbol == elem.get_symbol();
        });
        
        if (pos != sh_vec->end()){ // asset already issued in blockchain
            continue;
        } 
        // symbol filter
        if(!argument_.symbol.empty() && argument_.symbol !=  symbol)
            continue;
        Json::Value asset_data;
        asset_data["symbol"] = elem.detail.get_symbol();
        asset_data["address"] = "";
        symbol = elem.detail.get_symbol();
        asset_data["quantity"] += elem.detail.get_maximum_supply();
        asset_data["decimal_number"] += elem.detail.get_decimal_number();
        //asset_data["address"] = "";
        asset_data["status"] = "unissued";
        assets.append(asset_data);
    }

    if (get_api_version() == 1 && assets.isNull()) { //compatible for v1
        aroot["assets"] = "";
    } else {
        aroot["assets"] = assets;
    }
    
    return console_result::okay;
}

} // namespace commands
} // namespace explorer
} // namespace libbitcoin

