


#include "Lua/Interface.h"
#include "steamworks_157/sdk/public/steam/steam_gameserver.h"
#include <thread>
#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <deque>
#include <mutex>
#include <format>
#include <atomic>
 

#ifdef __linux__
    #include <linux/limits.h>
    #define _MAX_PATH PATH_MAX
#endif







class GMSV{
public: 


    //THE ENTIRE MODULE IS UNLOADED INCLUDING ALL STATIC CLASSES ETC FOR EACH SERVER UNLOAD

    template<typename anyMsgCode, typename anyThreadCode>
    static void addDebugMsg(anyMsgCode code, 
        std::string msg,
        anyThreadCode threadCode){
        std::lock_guard lock(GMSV::mutex);
        debugMsgs.push_back(std::string(typeid(anyMsgCode).name())+" message code: "+std::to_string(code)+" message: "+msg+" | "+std::string(typeid(anyThreadCode).name())+" thread code: "+std::to_string(threadCode));
       
    }

    static void runHook(GarrysMod::Lua::ILuaBase* lua, const char* eventName, const char* id,int (*func)(lua_State*)){
        lua->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
        lua->GetField(-1, "hook");
        lua->GetField(-1, "Add");
        lua->PushString(eventName);
        lua->PushString(id);
        lua->PushCFunction(func);
        lua->Call(3, 0);
        lua->Pop(2);
    }

    static void removeHook(GarrysMod::Lua::ILuaBase* lua,
         const char* eventName, const char* id){
        lua->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
        lua->GetField(-1, "hook");
        lua->GetField(-1, "Remove");
        lua->PushString(eventName);
        lua->PushString(id);
        lua->Call(2, 0);
        lua->Pop(2);
    }

    
    
    
    static void init(GarrysMod::Lua::ILuaBase* lua,void(*initCallback)(bool,GarrysMod::Lua::ILuaBase*),void(*shutDownCallback)(GarrysMod::Lua::ILuaBase*)){


        runHook(lua,"Think","GMSVDebug",printDebugMsgs);

        ready = true;
        //TODO: should only create this once per server cycle and not again when something goes wrong
        if (!gmsv){
            

            lua->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
            lua->GetField(-1,"game");
            lua->GetField(-1,"SinglePlayer");
            lua->Call(0,1);

          
            if (lua->GetBool(-1)){
                initCallback(true,lua);
                if (GMSV::isSteamServerInit){
                    gmsv = new GMSV();
                    std::thread t([]{
    
                        do{
                            SteamGameServer_RunCallbacks();
                            std::this_thread::sleep_for(std::chrono::milliseconds(10));

                        }while(!exit);
                        exit = false;
        
                    });
                    t.detach();
                }
                
            }else{
                gmsv = new GMSV();
                initCallback(false,lua);
            }
            
            lua->Pop(3);
        }else{
            initCallback(false,lua);
        }
        
        
    
        

        GMSV::initCallback = initCallback;
        GMSV::shutDownCallback = shutDownCallback;
       
        
        
       
        
    }

    

    static void shutDown(GarrysMod::Lua::ILuaBase* lua){

        shutDownCallback(lua);
         delete gmsv;
         gmsv = nullptr;
         if (isSteamServerInit){
            //exitingRunSteamCallbacks = false;
            exit = true;
            while (exit)
            {

            }
         }
         //FIX: this gets set to unInint whenever we lose connection the the steam server
         
    }
    static bool isSteamServerInit;

    
    static void (*initCallback)(bool,GarrysMod::Lua::ILuaBase*);
    static void(*shutDownCallback)(GarrysMod::Lua::ILuaBase*);
    

    static bool isReady(){
        return GMSV::ready;
    }
    

     static const char* gmodModuleName;
private:
    STEAM_GAMESERVER_CALLBACK(GMSV,steamServerConnectFail, SteamServerConnectFailure_t);
    STEAM_GAMESERVER_CALLBACK(GMSV,steamServerDisconnected,SteamServersDisconnected_t);

    static std::vector<std::string> debugMsgs; 

   
    static int printDebugMsgs(lua_State* luaState){
        if (debugMsgs.size() > 0){
             std::lock_guard lock(mutex);
            luaState->luabase->PushSpecial( GarrysMod::Lua::SPECIAL_GLOB ); 
            luaState->luabase->GetField( -1, "print" );
            std::string msg;
            std::string fromGmodModuleName = "from "+std::string(gmodModuleName)+": ";
            msg = fromGmodModuleName+debugMsgs[0];
            for(int msgI = 1; msgI < debugMsgs.size(); msgI++){
                msg+= "\n"+fromGmodModuleName+debugMsgs[msgI];
            }
            luaState->luabase->PushString(msg.c_str());
            luaState->luabase->Call( 1, 0 ); 
            luaState->luabase->Pop();
            debugMsgs.clear();
        }

        return 0;
       
        
    }
    static std::atomic<bool> ready;
    static bool exit;
    static GMSV* gmsv;
    static std::mutex mutex;
     

    
};




void GMSV::steamServerConnectFail(SteamServerConnectFailure_t* res){
    if (res->m_bStillRetrying){
        return;
    }

    ready = false;

}

void GMSV::steamServerDisconnected(SteamServersDisconnected_t*){
    ready = false;
}


GMSV* GMSV::gmsv;
void (*GMSV::initCallback)(bool,GarrysMod::Lua::ILuaBase*);
void(*GMSV::shutDownCallback)(GarrysMod::Lua::ILuaBase*);
bool GMSV::exit;
bool GMSV::isSteamServerInit;
std::atomic<bool> GMSV::ready;
std::mutex GMSV::mutex;
std::vector<std::string> GMSV::debugMsgs;

class GMSVWorkshop {
public:
    
    
     static int init(GarrysMod::Lua::ILuaBase* lua){

          
        run = [](lua_State* luaState){
            if (!isWorkshopInit(luaState->luabase)){
                return 0;
            }
            
           
             //if (downloading.size() > 0){
            GMSV::runHook(luaState->luabase,"Think","GMSVWorkshopRun",run);
           // }
            return 0;
        };

        if (downloading.size() > 0){
             GMSV::runHook(lua,"Think","GMSVWorkshopRun",run);
        }

        GMSV::addDebugMsg(tryInit,"Trying to init",main);
       
        lua->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
        lua->GetField(-1, "steamworks");
        lua->PushCFunction(downloadServerUGC);
        lua->SetField(-2, "DownloadUGC");
        lua->Pop(2);

        
        return 0;
    }

   

    static void shutDown(GarrysMod::Lua::ILuaBase* lua){
        GMSV::removeHook(lua,"Think","GMSVWorkshopRun");
        delete workshop;
        workshop = nullptr;
        //pending.clear();
        //finished.clear();
        for (auto pair = downloading.begin(); pair != downloading.end(); ++pair){
            downloading.erase(pair);
            delete pair->second;
        }
    }


     


private:


    
   
    

   // STEAM_GAMESERVER_CALLBACK(GMSVWorkshop, onLoginSuccess, SteamServersConnected_t);
    
    STEAM_GAMESERVER_CALLBACK(GMSVWorkshop, onDownloadItem, DownloadItemResult_t);
    
    static void nullCallback(GarrysMod::Lua::ILuaBase* lua){
        lua->PushNil();
        lua->PushNil();
        lua->PCall(2,0,0);
    }
    class Item {
    public:
       
        int downloadCallbackRef;
        uint64 workshopId;
        EResult downloadResult;
     //   std::filesystem::path loadPath;
        Item(int downloadCallbackRef) {
            this->downloadCallbackRef = downloadCallbackRef;
   
        }


        
    };

    //static void(*hello)(DownloadItemResult_t*);
    static std::vector<Item*> pending;

    
    static std::mutex mutex;
    static std::unordered_map<uint64, Item*> downloading;
   
    static std::deque<Item*> finished;
   
    static GMSVWorkshop* workshop;

    static int(*run)(lua_State*);
  


    
    

    static bool isWorkshopInit(GarrysMod::Lua::ILuaBase* lua){
        
        
         
        
        if (!SteamGameServer() || !SteamGameServer()->BLoggedOn()){
            return 0;
        }

        if (!workshop){
            workshop = new GMSVWorkshop();
            if (!SteamGameServerUGC()->BInitWorkshopForGameServer(4000,(std::filesystem::current_path() / "garrysmod/cache/gmsv_workshop").string().c_str())){
                //GMSV::fail();
                GMSV::removeHook(lua,"Think","GMSVWorkshopRun");
                delete workshop;
                workshop = nullptr;
                GMSV::init(lua,GMSV::initCallback,GMSV::shutDownCallback);
                return 0;
            }
        }
         //SteamGameServer()->LogOff();
         run = runCallbacks;
        

    
       

        

        return 1;
    }

  
    
    enum WorkshopErrCodes{
        notGetAddon,
        gotAddon,
        tryInit
    };

    enum WorkshopThreadCode{
        main
    };

   


    static int runCallbacks(lua_State* luaState) {


        //TODO fix this initState is no longer 0 in UnInit mode 

        
        
        if (!GMSV::isReady()){
                
           
            GMSV::removeHook(luaState->luabase,"Think","GMSVWorkshopRun");
            GMSV::init(luaState->luabase,GMSV::initCallback,GMSV::shutDownCallback);
            return 0;
        }
        std::lock_guard lock(mutex);
        Item* item;
        
         while (pending.size() > 0)
        {
            item = pending.back();
            if (!SteamGameServerUGC()->DownloadItem(item->workshopId,false)){

                luaState->luabase->ReferencePush(item->downloadCallbackRef);
                nullCallback(luaState->luabase);
                luaState->luabase->ReferenceFree(item->downloadCallbackRef);

                downloading.erase(item->workshopId);
                delete item;
            }
            pending.pop_back();
        }

            

        
    
        if (finished.size() > 0){
            int fileRef;
            std::string path;
            char downloadFolder[_MAX_PATH];

            item = finished.front();
            luaState->luabase->ReferencePush(item->downloadCallbackRef);
            if (item->downloadResult == k_EResultOK){
                if (SteamGameServerUGC()->GetItemInstallInfo(item->workshopId, nullptr, downloadFolder,sizeof(downloadFolder),nullptr)) {
                        
                      if (std::filesystem::is_directory(downloadFolder)){
                                auto iter = std::filesystem::directory_iterator(downloadFolder);
                                if (iter->exists()){
                                    path = std::filesystem::relative(iter->path(),std::filesystem::current_path() / "garrysmod/").string();
                                    
                                }else{
                                    item->downloadResult = k_EResultFail;
                                    
                                }
                            }else{
                                path = std::filesystem::relative(downloadFolder,std::filesystem::current_path() / "garrysmod/").string();
                            }
                            
                            if (item->downloadResult == k_EResultOK){
                                
          
                                luaState->luabase->PushSpecial(GarrysMod::Lua::SPECIAL_GLOB);
                                        luaState->luabase->GetField(-1,"file");
                                        luaState->luabase->GetField(-1,"Open");
                                        luaState->luabase->PushString(path.c_str());
                                        luaState->luabase->PushString("rb");
                                        luaState->luabase->PushString("GAME");
                                        luaState->luabase->Call(3,1);
                                        
                                        if (luaState->luabase->GetType(-1) != GarrysMod::Lua::Type::File){
                                            luaState->luabase->Pop(3);
                                            item->downloadResult = k_EResultFail;
                                            
                                        }else{
                                            fileRef = luaState->luabase->ReferenceCreate();
                                        luaState->luabase->Pop(2); //pop 2
                                            luaState->luabase->PushString(path.c_str());
                                            luaState->luabase->ReferencePush(fileRef);
                                            luaState->luabase->PCall(2,0,0);
                                            luaState->luabase->ReferencePush(fileRef);
                                            luaState->luabase->GetField(-1,"Close");
                                            luaState->luabase->ReferencePush(fileRef);
                                            luaState->luabase->Call(1,0);
                                            luaState->luabase->Pop();
                                            luaState->luabase->ReferenceFree(fileRef);
                                            luaState->luabase->PushSpecial( GarrysMod::Lua::SPECIAL_GLOB ); // Push the global table
                                            
                                            GMSV::addDebugMsg(gotAddon,"Got addon",main);
                                        }
     
                                    
                            }
                            
                }
                

                

            }
            if (item->downloadResult != k_EResultOK){
                
                nullCallback(luaState->luabase);
                GMSV::addDebugMsg(notGetAddon,"could not get addon reason: "+std::to_string(item->downloadResult),main);
            }

        

            
            luaState->luabase->ReferenceFree(item->downloadCallbackRef);
            finished.pop_front();

            downloading.erase(item->workshopId);
            delete item;
            

           

        }
        
       
        if (downloading.size() == 0) {
            GMSV::removeHook(luaState->luabase,"Think","GMSVWorkshopRun");
        }
       

            
        
            
            
        
       
        
    
    

    

    
       return 0;
   }

   static int downloadServerUGC(lua_State* luaState) {

       

       

        luaState->luabase->CheckType(1, GarrysMod::Lua::Type::String);

        luaState->luabase->CheckType(2, GarrysMod::Lua::Type::Function);



      
       
        
       uint64 workshopId;

       try {
           workshopId = std::stoul(luaState->luabase->GetString(1));
       }
       catch (std::invalid_argument) {
           nullCallback(luaState->luabase);
           return 0;
       }
       catch (std::out_of_range) {
           nullCallback(luaState->luabase);
           return 0;
       }
        std::lock_guard lock(mutex);

   
       if (downloading[workshopId]){
            nullCallback(luaState->luabase);
            return 0;
       }
       Item* item;


       item = new Item(luaState->luabase->ReferenceCreate());

    //   item->loadPath = loadPath;
        item->workshopId = workshopId;
        downloading[workshopId] = item;

       
       if (isWorkshopInit(luaState->luabase)){
            
            if (!SteamGameServerUGC()->DownloadItem(workshopId, false))
                {
                    luaState->luabase->ReferencePush(item->downloadCallbackRef);
                    nullCallback(luaState->luabase);
                    luaState->luabase->ReferenceFree(item->downloadCallbackRef);
                    downloading.erase(workshopId);
                    delete item;
                    return 0;
                }
       }else{
            pending.push_back(item);
       }

       if (downloading.size() == 1){

            GMSV::runHook(luaState->luabase,"Think","GMSVWorkshopRun",run);
       }

       
    


       return 0;

   };

   




};


std::unordered_map<uint64, GMSVWorkshop::Item*> GMSVWorkshop::downloading;
std::deque<GMSVWorkshop::Item*> GMSVWorkshop::finished;
std::mutex GMSVWorkshop::mutex;
GMSVWorkshop* GMSVWorkshop::workshop;
std::vector<GMSVWorkshop::Item*> GMSVWorkshop::pending;

int (*GMSVWorkshop::run)(lua_State*);








void GMSVWorkshop::onDownloadItem(DownloadItemResult_t* res) {


   
   
     //testId = item->downloadResult->m_nPublishedFileId;


     if (downloading.count(res->m_nPublishedFileId)){
        std::lock_guard lock(mutex);
        Item* item = downloading[res->m_nPublishedFileId];
        item->downloadResult = res->m_eResult;
        finished.push_back(item);
     }
    
   
    
}






const char* GMSV::gmodModuleName = "gmsv_workshop";
//bool initServer = true;
GMOD_MODULE_OPEN()
{

    

    

    GMSV::addDebugMsg(0,"Using released version 1.00",0);


    
    GMSV::init(LUA,[](bool initSteamServer, GarrysMod::Lua::ILuaBase* lua){
       
        GMSVWorkshop::init(lua);
        if (initSteamServer){
            if (!SteamGameServer()){
                if (!SteamGameServer_Init(0, 0, 0, EServerMode::eServerModeNoAuthentication,"1.0.0.0")){
                    return;
                }
            }
            
            GMSV::isSteamServerInit = true;
        }
        

        if (GMSV::isSteamServerInit){
            if(!SteamGameServer()->BLoggedOn()){
                SteamGameServer()->LogOnAnonymous();
            }
        }
        
    },[](GarrysMod::Lua::ILuaBase* lua){
        GMSVWorkshop::shutDown(lua);
    });


    //SteamGameServer()->LogOnAnonymous();
    

    //if (initWorkshop){


        
    
    return 0;
}



GMOD_MODULE_CLOSE()
{
   
    GMSV::shutDown(LUA);
    //GMSVWorkshop::shutDown(LUA);
    
 
   
    return 0;
}