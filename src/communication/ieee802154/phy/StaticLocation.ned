package cometos.src.communication.ieee802154.phy;

// Exactly as MiXiM BaseMobility, except that location info
// is read from the specified location file 
// An Addressing module has to be present and the same
// addresses used to identify the nodes.
simple StaticLocation
{
    parameters:
        xml locationFile;
        bool notAffectedByHostState = true;
        bool coreDebug; // debug switch for the core framework
}
