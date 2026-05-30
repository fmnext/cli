#pragma once
#include "preinclude.hpp"
#include "sqlite3.h"

namespace fmnext
{
    class SQLManager
    {
    public:
        SQLManager() = default;

        ~SQLManager()
        {
            if (mInstance != nullptr)
            {
                sqlite3_close(mInstance);
            }
        }

        SQLManager(const std::string& pPath, const std::string& pMediaName) : mPath(pPath), mMediaName(pMediaName)
        {
            printf("\tSQLite %s (%d) \n", SQLITE_VERSION, SQLITE_VERSION_NUMBER);
            //printf("%s %s %s \n\n", SQLITE_SCM_TAGS, SQLITE_SCM_BRANCH, SQLITE_SCM_DATETIME);
            
            if (std::filesystem::exists(pPath))
            {
                rc = sqlite3_open(mPath.c_str(), &mInstance);
                if (rc) {
                    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(mInstance));
                    sqlite3_close(mInstance);
                }
            }
        }

        std::shared_ptr<DataBaseRecords> GetQuery()
        {
            std::string query = "SELECT Data_Car.MediaName, ";
            query += "Data_Car.Id, ";
            query += "List_UpgradeCarBody.CarBodyID, ";
            query += "List_UpgradeTireCompound.TireModelName, ";
            query += "Data_Car.FrontTireWidthMM, ";
            query += "Data_Car.FrontTireAspect, ";
            query += "Data_Car.FrontWheelDiameterIN, ";
            query += "Data_Car.RearTireWidthMM, ";
            query += "Data_Car.RearTireAspect, ";
            query += "Data_Car.RearWheelDiameterIN, ";
            query += "Data_Car.Thumbnail, ";
            query += "Data_CarBody.ModelWheelbase, ";
            query += "Data_CarBody.ModelFrontTrackOuter, ";
            query += "Data_CarBody.ModelRearTrackOuter, ";
            query += "Data_CarBody.ModelFrontStockRideHeight, ";
            query += "Data_CarBody.ModelRearStockRideHeight, ";
            query += "Data_CarBody.BottomCenterWheelbasePosx, ";
            query += "Data_CarBody.BottomCenterWheelbasePosy, ";
            query += "Data_CarBody.BottomCenterWheelbasePosZ ";
            query += "FROM Data_Car ";
            query += "INNER JOIN List_UpgradeTireCompound ON List_UpgradeTireCompound.Ordinal = Data_Car.Id ";
            query += "INNER JOIN List_UpgradeCarBody ON List_UpgradeCarBody.Ordinal = Data_Car.Id ";
            query += "INNER JOIN Data_CarBody ON Data_CarBody.Id = List_UpgradeCarBody.CarBodyID ";
            query += "WHERE MediaName LIKE '";
            query += mMediaName;
            query += "' ";
            query += "AND List_UpgradeTireCompound.IsStock = 1 ";
            query += "ORDER BY List_UpgradeCarBody.CarBodyID";

            sqlite3_stmt* statement = nullptr;

            if (!mInstance && rc != SQLITE_OK)
            {
                fprintf(stderr, "SQL error: %s\n", sqlite3_errstr(rc));
            }

            if (mInstance)
            {
                rc = sqlite3_prepare_v3(mInstance, query.c_str(), -1, SQLITE_PREPARE_PERSISTENT, &statement, nullptr);

                if (!statement && rc != SQLITE_OK)
                {
                    fprintf(stderr, "SQL error: %s\n", sqlite3_errstr(rc));
                }

                if (rc == SQLITE_OK && sqlite3_step(statement) == SQLITE_ROW)
                {
                    m_records = std::make_shared<DataBaseRecords>();

                    m_records->MediaName = std::string(reinterpret_cast<const char*>(sqlite3_column_text(statement, 0)));
                    m_records->CarId = sqlite3_column_int(statement, 1);
                    m_records->CarBodyID = sqlite3_column_int(statement, 2);
                    m_records->TireModelName = std::string(reinterpret_cast<const char*>(sqlite3_column_text(statement, 3)));
                    m_records->fallback = false;
                    m_records->FrontTireWidthMM = sqlite3_column_int(statement, 4);
                    m_records->FrontTireAspect = sqlite3_column_int(statement, 5);
                    m_records->FrontWheelDiameterIN = sqlite3_column_int(statement, 6);
                    m_records->RearTireWidthMM = sqlite3_column_int(statement, 7);
                    m_records->RearTireAspect = sqlite3_column_int(statement, 8);
                    m_records->RearWheelDiameterIN = sqlite3_column_int(statement, 9);
                    m_records->Thumbnail = std::string(reinterpret_cast<const char*>(sqlite3_column_text(statement, 10)));
                    m_records->ModelWheelbase = static_cast<float>(sqlite3_column_double(statement, 11));
                    m_records->ModelFrontTrackOuter = static_cast<float>(sqlite3_column_double(statement, 12));
                    m_records->ModelRearTrackOuter = static_cast<float>(sqlite3_column_double(statement, 13));
                    m_records->ModelFrontStockRideHeight = static_cast<float>(sqlite3_column_double(statement, 14));
                    m_records->ModelRearStockRideHeight = static_cast<float>(sqlite3_column_double(statement, 15));
                    m_records->BottomCenterWheelbasePosX = static_cast<float>(sqlite3_column_double(statement, 16));
                    m_records->BottomCenterWheelbasePosY = static_cast<float>(sqlite3_column_double(statement, 17));
                    m_records->BottomCenterWheelbasePosZ = static_cast<float>(sqlite3_column_double(statement, 18));

                    sqlite3_finalize(statement);

                    return m_records;
                }
            }

            //rc = sqlite3_exec(m_db, query.c_str(), callback, 0, &zErrMsg);

            return nullptr;
        }

    private:
        int rc = SQLITE_CANTOPEN;
        sqlite3* mInstance = nullptr;
        std::shared_ptr<DataBaseRecords> m_records = nullptr;
        std::string mPath, mMediaName;    
        //static std::unordered_map<std::string, std::string> sql_data;

        static int callback(void* NotUsed, int argc, char** argv, char** azColName)
        {
            for (int i = 0; i < argc; i++) {
                //sql_data.try_emplace(azColName[i], argv[i]);
                printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
            }
            return 0;
        }
    };
}
