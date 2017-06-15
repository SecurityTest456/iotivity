/******************************************************************
 *
 * Copyright 2015 Samsung Electronics All Rights Reserved.
 *
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************/

#include "simulator_manager.h"

std::vector<SimulatorSingleResourceSP> g_singleResources;
std::vector<SimulatorCollectionResourceSP> g_collectionResources;

class AppLogger : public ILogger
{
    public:
        void write(std::string time, ILogger::Level level, std::string message)
        {
            std::cout << "[APPLogger] " << time << " " << ILogger::getString(level) << " "
                      << message;
        }
};
std::shared_ptr<AppLogger> gAppLogger(new AppLogger());

int selectResource()
{
    if (0 == g_singleResources.size())
    {
        std::cout << "No resouces!" << std::endl;
        return -1;
    }

    int index = 1;
    for (auto &resource : g_singleResources)
    {
        std::cout << index++ << ": " << resource->getURI().c_str() << std::endl;
    }

    int choice = -1;
    std::cout << "Choose the resource: ";
    std::cin >> choice;

    if (choice < 1 || choice > index - 1)
    {
        std::cout << "Invalid choice !" << std::endl;
        choice = -1;
    }

    return choice;
}

void simulateResource()
{
    try
    {
        // Resource model change callback
        SimulatorResource::ResourceModelUpdateCallback modelChangeCB =
            [](const std::string & uri, const SimulatorResourceModel & resModel)
        {
            std::cout << "[callback] Resource model is changed URI: " << uri.c_str() << std::endl;
            std::cout << "#### Modified attributes are ####" << std::endl;
            std::cout << "#### Updated resource model ####" << std::endl;
            std::cout << resModel.asString() << std::endl;
            std::cout << "########################" << std::endl;
        };

        // Observer added/removed callback
        SimulatorResource::ObserverCallback observerCB =
            [] (const std::string & uri, ObservationStatus state, const ObserverInfo & observerInfo)
        {
            std::cout << "[callback] Observer notification received..." << uri << std::endl;

            std::ostringstream out;
            out << "ID:  " << (int) observerInfo.id << std::endl;
            out << " [address: " << observerInfo.address << " port: " << observerInfo.port
                << "]" << std::endl;
            out << "State: " << ((state == ObservationStatus::REGISTER) ? "REGISTER" : "UNREGISTER") <<
                std::endl;
            std::cout << out.str();
        };

        // Get the RAML file path from user
        std::string configPath;
        std::cout << "Enter RAML path: ";
        std::cin >> configPath;

        SimulatorResourceSP resource =
            SimulatorManager::getInstance()->createResource(configPath);

        // Add resource to appropriate list
        if (SimulatorResource::Type::SINGLE_RESOURCE == resource->getType())
        {
            std::cout << "Single type resource created [URI:  " << resource->getURI() << " ]" << std::endl;
            SimulatorSingleResourceSP singleRes =
                std::dynamic_pointer_cast<SimulatorSingleResource>(resource);
            if (!singleRes)
            {
                std::cout << "Error occured while converting SimulatorResource to SimulatorSingleResource!" << std::endl;
                return;
            }

            singleRes->setModelChangeCallback(modelChangeCB);
            singleRes->setObserverCallback(observerCB);
            g_singleResources.push_back(singleRes);
        }
        else
        {
            std::cout << "Collection type resource created [URI:  " << resource->getURI() << " ]" << std::endl;
            SimulatorCollectionResourceSP collectionRes =
                std::dynamic_pointer_cast<SimulatorCollectionResource>(resource);
            if (!collectionRes)
            {
                std::cout << "Error occured while converting SimulatorResource to SimulatorCollectionResource!" << std::endl;
                return;
            }

            collectionRes->setObserverCallback(observerCB);
            g_collectionResources.push_back(collectionRes);
        }
    }
    catch (InvalidArgsException &e)
    {
        std::cout << "InvalidArgsException occured [code : " << e.code() << " Details: "
                  << e.what() << "]" << std::endl;
    }
    catch (SimulatorException &e)
    {
        std::cout << "SimulatorException occured [code : " << e.code() << " Details: "
                  << e.what() << "]" << std::endl;
    }
}

void displayResource()
{
    int index = selectResource();
    if (-1 == index)
        return;

    SimulatorSingleResourceSP resource = g_singleResources[index - 1];

    std::cout << "#############################" << std::endl;
    std::cout << "Name: " << resource->getName() << std::endl;
    std::cout << "URI: " << resource->getURI() << std::endl;
    std::cout << "Resource type: " << resource->getResourceType() << std::endl;
    std::cout << "Interface type:";
    for (auto &interfaceType : resource->getInterface())
        std::cout << " " << interfaceType << std::endl;

    // Attributes
    std::cout << "##### Representation #####" << std::endl;
    std::cout << resource->getResourceModel().asString() << std::endl;
    std::cout << "#############################" << std::endl;
}

void startResource()
{
    int index = selectResource();
    if (-1 == index)
        return;

    SimulatorSingleResourceSP resource = g_singleResources[index - 1];
    resource->start();
    std::cout << "Resource started!" << std::endl;
}

void stopResource()
{
    int index = selectResource();
    if (-1 == index)
        return;

    SimulatorSingleResourceSP resource = g_singleResources[index - 1];
    resource->stop();
    std::cout << "Resource stopped!" << std::endl;
}

void automateResourceUpdate()
{
    SimulatorSingleResource::AutoUpdateCompleteCallback callback =
        [](const std::string & uri, const int id)
    {
        std::cout << "Update automation is completed [URI: " << uri
                  << "  AutomationID: " << id << "] ###" << std::endl;
    };

    int index = selectResource();
    if (-1 == index)
        return;

    AutoUpdateType type = AutoUpdateType::ONE_TIME;
    int choice = 0;
    std::cout << "Press 1 if you want recurrent automation: ";
    std::cin >> choice;
    if (1 == choice)
        type = AutoUpdateType::REPEAT;

    try
    {
        int id = g_singleResources[index - 1]->startResourceUpdation(type, -1, callback);

        std::cout << "startUpdateAutomation() returned succces : " << id << std::endl;
    }
    catch (SimulatorException &e)
    {
        std::cout << "SimulatorException occured [code : " << e.code() << " Details: " <<
                  e.what() << "]" << std::endl;
    }
}

void automateAttributeUpdate()
{
    SimulatorSingleResource::AutoUpdateCompleteCallback callback =
        [](const std::string & uri, const int id)
    {
        std::cout << "Update automation is completed [URI: " << uri
                  << "  AutomationID: " << id << "] ###" << std::endl;
    };

    int index = selectResource();
    if (-1 == index)
        return;

    SimulatorSingleResourceSP resource = g_singleResources[index - 1];
    std::map<std::string, SimulatorResourceAttribute> attributes =
        resource->getAttributes();
    int size = 0;
    for (auto &attributeEntry : attributes)
    {
        std::cout << ++size << ": " << attributeEntry.first << std::endl;
    }

    if (0 == size)
    {
        std::cout << "This resource doest not contain any attributes!" << std::endl;
        return;
    }

    int choice = -1;
    std::cout << "Select the attribute which you want to automate for updation: " <<
              std::endl;
    std::cin >> choice;
    if (choice < 0 || choice > size)
    {
        std::cout << "Invalid selection!" << std::endl;
        return;
    }

    int count = 0;
    std::string attributeName;
    for (auto &attributeEntry : attributes)
    {
        if (count == choice - 1)
        {
            attributeName = attributeEntry.first;
            break;
        }

        count++;
    }

    AutoUpdateType type = AutoUpdateType::ONE_TIME;
    std::cout << "Press 1 if you want recurrent automation: ";
    std::cin >> choice;
    if (1 == choice)
        type = AutoUpdateType::REPEAT;

    std::cout << "Requesting attribute automation for " << attributeName <<
              std::endl;

    try
    {
        int id = resource->startAttributeUpdation(attributeName, type, -1, callback);
        std::cout << "startUpdateAutomation() returned succces : " << id << std::endl;
    }
    catch (SimulatorException &e)
    {
        std::cout << "SimulatorException occured [Error: " << e.code() << " Details: " <<
                  e.what() << "]" << std::endl;
    }
}

void stopAutomation()
{
    int index = selectResource();
    if (-1 == index)
        return;

    SimulatorSingleResourceSP resource = g_singleResources[index - 1];

    // Select the automation to stop
    std::vector<int> ids;
    {
        std::vector<int> rids = resource->getResourceUpdations();
        std::vector<int> aids = resource->getAttributeUpdations();
        ids.insert(ids.end(), rids.begin(), rids.end());
        ids.insert(ids.end(), aids.begin(), aids.end());
    }

    if (!ids.size())
    {
        std::cout << "No automation operation is going on this resource right now!" <<
                  std::endl;
        return;
    }

    for (auto &id : ids)
    {
        std::cout <<  id  << " ";
        resource->stopUpdation(id);
    }
}

void getObservers()
{
    int index = selectResource();
    if (-1 == index)
        return;

    SimulatorSingleResourceSP resource = g_singleResources[index - 1];

    std::vector<ObserverInfo> observersList = resource->getObservers();

    std::cout << "##### Number of Observers [" << observersList.size() << "]" << std::endl;
    for (auto &observerInfo : observersList)
    {
        std::cout << " ID :  " << (int) observerInfo.id << " [address: " <<
                  observerInfo.address << " port: " << observerInfo.port << "]" << std::endl;
    }
    std::cout << "########################" << std::endl;
}

void printMainMenu()
{
    std::cout << "############### MAIN MENU###############" << std::endl;
    std::cout << "1. Simulate resource" << std::endl;
    std::cout << "2. Display resource information" << std::endl;
    std::cout << "3. Start resource" << std::endl;
    std::cout << "4. Stop resource" << std::endl;
    std::cout << "5. Automate resource update" << std::endl;
    std::cout << "6. Automate attributes update" << std::endl;
    std::cout << "7. Stop Automation" << std::endl;
    std::cout << "8. Get Observers of a resource" << std::endl;
    std::cout << "9. Set Logger" << std::endl;
    std::cout << "10. Set Device Info" << std::endl;
    std::cout << "11. Set Platform Info" << std::endl;
    std::cout << "12. Add Interface" << std::endl;
    std::cout << "13. Add Resource Manually" << std::endl;
    std::cout << "14. Change Rsource Attribute Manually" << std::endl;
    std::cout << "15. PrintMenu" << std::endl;
    std::cout << "16. Help" << std::endl;
    std::cout << "0. Exit" << std::endl;
    std::cout << "######################################" << std::endl;
}

void setLogger()
{
    std::cout << "1. Default console logger" << std::endl;
    std::cout << "2. Default file logger" << std::endl;
    std::cout << "3. custom logger" << std::endl;

    int choice = -1;
    std::cin >> choice;
    if (choice <= 0 || choice > 3)
    {
        std::cout << "Invalid selection !" << std::endl;
        return;
    }

    switch (choice)
    {
        case 1:
            {
                if (false == SimulatorManager::getInstance()->setConsoleLogger())
                    std::cout << "Failed to set the default console logger" << std::endl;
            } break;
        case 2:
            {
                std::string filePath;
                std::cout << "Enter the file path (without file name) : ";
                std::cin >> filePath;
                if (false == SimulatorManager::getInstance()->setFileLogger(filePath))
                    std::cout << "Failed to set default file logger" << std::endl;
            } break;
        case 3: SimulatorManager::getInstance()->setLogger(gAppLogger);
    }
}

void setDeviceInfo()
{
    try
    {
        SimulatorManager::getInstance()->setDeviceInfo("IoTivity Simulator Linux Sample",
                                                       "c49f7cba-3b3d-490b-9d3f-d4f17a3dad26");
        std::cout << "Setting Device Info is successful" << std::endl;
    }
    catch (InvalidArgsException &e)
    {
        std::cout << "InvalidArgsException occured [code : " << e.code() << " Details: "
                  << e.what() << "]" << std::endl;
    }
    catch (SimulatorException &e)
    {
        std::cout << "SimulatorException occured [code : " << e.code() << " Details: "
                  << e.what() << "]" << std::endl;
    }
}

void setPlatformInfo()
{
    PlatformInfo pInfo;
    pInfo.setPlatformID("Samsung Platform Identifier");
    pInfo.setFirmwareVersion("FirwareVersion01");
    pInfo.setHardwareVersion("HardwareVersion01");
    pInfo.setManufacturerName("Samsung");
    pInfo.setManufacturerUrl("www.samsung.com");
    pInfo.setModelNumber("Samsung Model Num01");
    pInfo.setOSVersion("OSVersion01");
    pInfo.setPlatformVersion("PlatformVersion01");
    pInfo.setSupportUrl("http://www.samsung.com/support");
    pInfo.setSystemTime("2015-09-10T11:10:30Z");
    pInfo.setDateOfManfacture("2015-09-10T11:10:30Z");

    try
    {
        SimulatorManager::getInstance()->setPlatformInfo(pInfo);
        std::cout << "Setting Platform Info is successful" << std::endl;
    }
    catch (SimulatorException &e)
    {
        std::cout << "SimulatorException occured [code : " << e.code() << " Details: "
                  << e.what() << "]" << std::endl;
    }
}

void addInterface()
{

    int index = selectResource();
    if (-1 == index)
        return;

    SimulatorSingleResourceSP resource = g_singleResources[index - 1];
    resource->addInterface(OC_RSRVD_INTERFACE_SENSOR);
    resource->addInterface(OC_RSRVD_INTERFACE_ACTUATOR);
}

void modelChangeCall(const std::string & uri, const SimulatorResourceModel & resModel)
{
    std::cout << "########################" << std::endl;
    std::cout << "[callback] Resource model is changed URI: " << uri.c_str() << std::endl;
    std::cout << "#### Modified attributes are ####" << std::endl;
    std::cout << "#### Updated resource model ####" << std::endl;
    std::cout << resModel.asString() << std::endl;
    std::cout << "########################" << std::endl;
}

void observerCall(const std::string & uri, ObservationStatus state, const ObserverInfo & observerInfo)
{
    std::cout << "########################" << std::endl;
    std::cout << "[callback] Observer notification received..." << uri << std::endl;
    std::ostringstream out("",std::ios_base::out);
    out << "ID:  " << (int) observerInfo.id << std::endl;
    out << " [address: " << observerInfo.address << " port: " << observerInfo.port << "]" << std::endl;
    out << "State: " << ((state == ObservationStatus::REGISTER) ? "REGISTER" : "UNREGISTER") << std::endl;
    std::cout << out.str() << std::endl;
}

bool validateData(const std::string & str1,const std::string & str2,const std::string & str3)
{
    if ((str1.empty() == true) || (str2.empty() == true) || (str3.empty() == true))
    {
	    return false;
    }
    return true;
}

void addResource()
{
    try
    {
        std::string name,uri,type;
        std::cout << "Enter Resource Name" << std::endl;
        std::cin >> name;
        std::cout << "Enter Resource URI" << std::endl;
        std::cin >> uri;
        std::cout << "Enter Resource Type " << std::endl;
        std::cin >> type;
        if (false == validateData(name,uri,type))
        {
            std::cout << "Could not process ur request\n Wrong Input is entered" << std::endl;
            return;
        }
        
        SimulatorResourceSP resource = SimulatorManager::getInstance()->createSingleResource(name,uri,type);
        SimulatorResource::ResourceModelUpdateCallback modelChangeCB = modelChangeCall;

        // Observer added/removed callback
        SimulatorResource::ObserverCallback observerCB = observerCall;

        if (SimulatorResource::Type::SINGLE_RESOURCE == resource->getType())
        {
            std::cout << "Single type resource created [URI:  " << resource->getURI() << " ]" << std::endl;
            SimulatorSingleResourceSP singleRes = std::dynamic_pointer_cast<SimulatorSingleResource>(resource);
            if (!singleRes)
            {
                std::cout << "Error occured while converting SimulatorResource to SimulatorSingleResource!" << std::endl;
                return;
            }
            singleRes->setModelChangeCallback(modelChangeCB);
            singleRes->setObserverCallback(observerCB);
            g_singleResources.push_back(singleRes);
        }
    }
    catch (InvalidArgsException &e)
    {
        std::cout << "InvalidArgsException occured [code : " << e.code() << " Details: "
                  << e.what() << "]" << std::endl;
    }
    catch (SimulatorException &e)
    {
        std::cout << "SimulatorException occured [code : " << e.code() << " Details: "
                  << e.what() << "]" << std::endl;
    }
}

void updateAttr(SimulatorSingleResourceSP resource)
{
    SimulatorResourceAttribute attribute;
    std::string name;
    std::cout << "Enter Atribute name to get update" << std::endl;
    std::cout << resource->getResourceModel().asString() << std::endl;
    std::cin >> name;
        
    bool result;
    result = resource->getAttribute(name,attribute);
    if(result==0)
    {
        std::cout << "Entered attribute does not exist" << std::endl;
        return;
    }    
    if (attribute.getProperty()->isInteger())
    {
        int iVal;
        std::cout << "Enter integer value" << std::endl;
        std::cin >> iVal;
        while(std::cin.fail())
        {
            std::cout << "Integer value not entered" << std::endl;
            std::cout << "Enter Integer Value" << std::endl;
            std::cin.clear();
            std::cin.ignore(256,'\n');
            std::cin >> iVal;
        }               
        attribute.setValue(iVal);
        result=resource->updateAttributeValue(attribute,true);
    }    
    if (attribute.getProperty()->isString())
    {
        std::string sVal;
        std::cout << "Enter string value" << std::endl;
        std::cin >> sVal;
        attribute.setValue(sVal);
        result = resource->updateAttributeValue(attribute,true);
    }    
    if (attribute.getProperty()->isBoolean())
    {
        bool bVal;
        std::cout << "Enter boolean value" << std::endl;
        std::cin >> bVal;
        while(std::cin.fail())
        {
            std::cout << "Boolean value not entered" << std::endl;
            std::cout << "Enter Boolean Value" << std::endl;
            std::cin.clear();
            std::cin.ignore(256,'\n');
            std::cin >> bVal;
        }
        attribute.setValue(bVal);
        result = resource->updateAttributeValue(attribute,true);
    } 
    if (result)
        std::cout << "Attribute added successfully" << std::endl;
    else
        std::cout << "Unable to add Attribute" << std::endl;

    return;
}

void addAttr(SimulatorSingleResourceSP resource)
{
    SimulatorResourceAttribute attribute;
    std::string name;
    int choice;
    
    std::cout << "Enter Attribute Name" << std::endl;
    std::cin >> name;
    
    std::map<std::string, SimulatorResourceAttribute> attributes = resource->getAttributes();
    for (auto &attributeEntry : attributes)
    {
        if (attributeEntry.first == name)
        {
            std::cout << attributeEntry.first << std::endl;
            std::cout << "Entered Attribute already exist" << std::endl;
            return;
        }
    }
    std::cout << "Enter the property type" << std::endl;
    std::cout << "1.Integer" << std::endl;
    std::cout << "2.String" << std::endl;
    std::cout << "3.Boolean" << std::endl;
    std::cin >> choice;
    
    switch(choice)
    {
        case 1:
            {
                int iVal;
                std::cout << "Enter value" << std::endl;
                std::cin >> iVal;
                while(std::cin.fail())
                {
                    std::cin.clear();
                    std::cin.ignore(256,'\n');
                    std::cout << "Enter the integer value" << std::endl;
                    std::cin >> iVal;
                }
                
                std::shared_ptr<IntegerProperty> ptr;
                ptr = IntegerProperty::build();
                attribute.setProperty(ptr);
                attribute.setValue(iVal);
            } break;
        case 2:
            {
                std::string sVal;
                std::cout << "Enter value" << std::endl;
                std::cin >> sVal;
                std::shared_ptr<StringProperty> ptr;
                ptr = StringProperty::build();
                attribute.setProperty(ptr);
                attribute.setValue(sVal);
            } break;
        case 3:
            {
                bool bVal;
                std::cout << "Enter value" << std::endl;
                std::cin >> bVal;
                while(std::cin.fail())
                {
                    std::cin.clear();
                    std::cin.ignore(256,'\n');
                    std::cout << "Enter the boolean value(0-1)" << std::endl;
                    std::cin >> bVal;
                }
                
                std::shared_ptr<BooleanProperty> ptr;
                ptr = BooleanProperty::build();
                attribute.setProperty(ptr);
                attribute.setValue(bVal);
            } break;
        default:
            {
                std::cout << "Invalid input" << std::endl;
            } break;
    }
    attribute.setName(name);
    bool result;
    result = resource->addAttribute(attribute, true);
    if (result)
        std::cout << "Attribute added successfully" << std::endl;
    else
        std::cout << "Unable to add Attribute" << std::endl;
    
    return;
}

void remAttr(SimulatorSingleResourceSP resource)
{
	 // Attributes
    std::cout << "##### Attributes of the selected resource #####" << std::endl;
    std::cout << resource->getResourceModel().asString() << std::endl;
    std::cout << "###############################################" << std::endl;
    
    std::string name;
    std::cout << "Enter the name of the Attribute you want to remove " << std::endl;
    std::cin >> name;
    
    bool result;
    result = resource->removeAttribute(name,true);    
    if (result)
        std::cout << "Attribute removed successfully" << std::endl;
    else
        std::cout << "Entered Attribute does not exist" << std::endl;
    
    return;
}

void changeAttribute()
{
    int index = selectResource();
    if (-1 == index)
    {
        return;
    }
    
    SimulatorSingleResourceSP resource = g_singleResources[index - 1];
    bool ret = true;
    while (ret)
    {
        std::cout << "1. Update Attribute value" << std::endl;
        std::cout << "2. Add New Attribute" << std::endl;
        std::cout << "3. Remove Attribute" << std::endl;
        std::cout << "0. Exit" << std::endl;
        
        int choice = -1;
        std::cout << "Enter your choice: ";
        std::cin >> choice;
        if (choice < 0 || choice > 3)
        {
            std::cout << "Invalid choice" << std::endl;
            continue;
        }
        switch (choice)
        {
            case 0:
                {
                    ret = false;
                    printMainMenu();
                } break;
            case 1:
                {
                    updateAttr(resource);
                } break;
            case 2:
                {
                    addAttr(resource);
                } break;
            case 3:
                {
                    remAttr(resource);
                } break;
            default:
                {
                    std::cout << "Invalid choice" << std::endl;
                } break;
        }
    }
}

int main(int argc, char *argv[])
{
    printMainMenu();
    bool cont = true;
    while (cont == true)
    {
        int choice = -1;
        std::cout << "Enter your choice: ";
        std::cin >> choice;
        if (choice < 0 || choice > 15)
        {
            std::cout << "Invaild choice !" << std::endl; continue;
        }

        switch (choice)
        {
            case 1 : simulateResource(); break;
            case 2 : displayResource(); break;
            case 3 : startResource(); break;
            case 4 : stopResource(); break;
            case 5 : automateResourceUpdate(); break;
            case 6 : automateAttributeUpdate(); break;
            case 7 : stopAutomation(); break;
            case 8 : getObservers(); break;
            case 9 : setLogger(); break;
            case 10: setDeviceInfo(); break;
            case 11: setPlatformInfo(); break;
            case 12: addInterface(); break;
            case 13: addResource(); break;
            case 14: changeAttribute(); break;
			case 15: printMainMenu(); break;
            case 0: cont = false;
        }
    }

    std::cout << "Terminating test !!!" << std::endl;
}
