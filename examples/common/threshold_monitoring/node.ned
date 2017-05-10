//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 


//import cometos.examples.simulation.base.Network;
//
package cometos.examples.common.threshold_monitoring;
//
// nötig???????????
//module tictoc extends Network
//{
//    
//    
//}

//import cometos.src.communication.traffic.PeriodicTraffic;
//import cometos.examples.common.tictoc.PeriodicTraffic;
import cometos.examples.simulation.base.Node154;


module Node extends Node154
{
    @display("bgb=304,354");
    submodules:

        handler : node_handler {
            @display("p=47,55");
        }
        
        
        
    connections:
        nic.gateIndOut --> handler.gateIndIn;
        handler.gateReqOut --> nic.gateReqIn;
        
}

