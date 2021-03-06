#include "robomongo/core/domain/MongoDatabase.h"

#include "robomongo/core/domain/MongoServer.h"
#include "robomongo/core/domain/MongoCollection.h"
#include "robomongo/core/events/MongoEvents.h"
#include "robomongo/core/AppRegistry.h"
#include "robomongo/core/EventBus.h"

namespace Robomongo
{

    R_REGISTER_EVENT(MongoDatabaseCollectionListLoadedEvent)
    R_REGISTER_EVENT(MongoDatabaseUsersLoadedEvent)
    R_REGISTER_EVENT(MongoDatabaseFunctionsLoadedEvent)
    R_REGISTER_EVENT(MongoDatabaseUsersLoadingEvent)
    R_REGISTER_EVENT(MongoDatabaseFunctionsLoadingEvent)
    R_REGISTER_EVENT(MongoDatabaseCollectionsLoadingEvent)

    MongoDatabase::MongoDatabase(MongoServer *server, const QString &name)
        :QObject(),_system(false),_server(server),_bus(AppRegistry::instance().bus())
    {
        _name = name;
        // Check that this is a system database
        _system = name == "admin" ||
        name == "local";
    }

    MongoDatabase::~MongoDatabase()
    {
        clearCollections();
    }

    void MongoDatabase::loadCollections()
    {
        _bus->publish(new MongoDatabaseCollectionsLoadingEvent(this));
        _bus->send(_server->client(), new LoadCollectionNamesRequest(this, _name));
    }

    void MongoDatabase::loadUsers()
    {
        _bus->publish(new MongoDatabaseUsersLoadingEvent(this));
        _bus->send(_server->client(), new LoadUsersRequest(this, _name));
    }

    void MongoDatabase::loadFunctions()
    {
        _bus->publish(new MongoDatabaseFunctionsLoadingEvent(this));
        _bus->send(_server->client(), new LoadFunctionsRequest(this, _name));
    }
    void MongoDatabase::createCollection(const QString &collection)
    {
        _bus->send(_server->client(), new CreateCollectionRequest(this, _name, collection));
    }

    void MongoDatabase::dropCollection(const QString &collection)
    {
        _bus->send(_server->client(), new DropCollectionRequest(this, _name, collection));
    }

    void MongoDatabase::renameCollection(const QString &collection, const QString &newCollection)
    {
        _bus->send(_server->client(), new RenameCollectionRequest(this, _name, collection, newCollection));
    }

    void MongoDatabase::duplicateCollection(const QString &collection, const QString &newCollection)
    {
        _bus->send(_server->client(), new DuplicateCollectionRequest(this, _name, collection, newCollection));
    }

    void MongoDatabase::createUser(const MongoUser &user, bool overwrite)
    {
        _bus->send(_server->client(), new CreateUserRequest(this, _name, user, overwrite));
    }

    void MongoDatabase::dropUser(const mongo::OID &id)
    {
        _bus->send(_server->client(), new DropUserRequest(this, _name, id));
    }

    void MongoDatabase::createFunction(const MongoFunction &fun)
    {
        _bus->send(_server->client(), new CreateFunctionRequest(this, _name, fun));
    }

    void MongoDatabase::updateFunction(const QString &name, const MongoFunction &fun)
    {
        _bus->send(_server->client(), new CreateFunctionRequest(this, _name, fun, name));
    }

    void MongoDatabase::dropFunction(const QString &name)
    {
        _bus->send(_server->client(), new DropFunctionRequest(this, _name, name));
    }

    void MongoDatabase::handle(LoadCollectionNamesResponse *loaded)
    {
        if (loaded->isError())
            return;

        clearCollections();
        const QList<MongoCollectionInfo> &colectionsInfos = loaded->collectionInfos();
        for(QList<MongoCollectionInfo>::const_iterator it = colectionsInfos.begin() ;it!=colectionsInfos.end();++it)    {
            const MongoCollectionInfo &info = *it;
            MongoCollection *collection = new MongoCollection(this, info);
            addCollection(collection);
        }

        _bus->publish(new MongoDatabaseCollectionListLoadedEvent(this, _collections));
    }

    void MongoDatabase::handle(LoadUsersResponse *event)
    {
        if (event->isError())
            return;
        
        _bus->publish(new MongoDatabaseUsersLoadedEvent(this, this, event->users()));
    }

    void MongoDatabase::handle(LoadFunctionsResponse *event)
    {
        if (event->isError())
            return;
            
        _bus->publish(new MongoDatabaseFunctionsLoadedEvent(this, this, event->functions()));
    }

    void MongoDatabase::clearCollections()
    {
        qDeleteAll(_collections);
        _collections.clear();
    }

    void MongoDatabase::addCollection(MongoCollection *collection)
    {
        _collections.append(collection);
    }
}
