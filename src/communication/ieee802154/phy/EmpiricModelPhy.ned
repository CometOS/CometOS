package cometos.src.communication.ieee802154.phy;

import org.mixim.modules.phy.PhyLayer;

simple EmpiricModelPhy extends PhyLayer
{
    parameters:
        xml linkStatsFile;
        double deciderCorrectionFactor = default(0);
        @class("EmpiricModelPhy");
}
