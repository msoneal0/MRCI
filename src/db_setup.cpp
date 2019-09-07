#include "db_setup.h"

//    This file is part of MRCI.

//    MRCI is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.

//    MRCI is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.

//    You should have received a copy of the GNU General Public License
//    along with MRCI under the LICENSE.md file. If not, see
//    <http://www.gnu.org/licenses/>.

bool setupDb(QString *errMsg)
{
    bool ret = true;

    errMsg->clear();

    Query query(QThread::currentThread());
    Query defaults(QThread::currentThread());

    if (query.inErrorstate())
    {
        ret = false;

        errMsg->append("database open failure: \n");
        errMsg->append(query.errDetail());
    }

    if (ret)
    {
        query.setType(Query::CREATE_TABLE, TABLE_IPHIST);
        query.addColumn(COLUMN_IPADDR);
        query.addColumn(COLUMN_TIME);
        query.addColumn(COLUMN_CLIENT_VER);
        query.addColumn(COLUMN_SESSION_ID);
        query.addColumn(COLUMN_LOGENTRY);

        ret = query.exec();
    }

    if (ret)
    {
        query.setType(Query::CREATE_TABLE, TABLE_MODULES);
        query.addColumn(COLUMN_MOD_NAME);
        query.addColumn(COLUMN_MOD_MAIN);
        query.addColumn(COLUMN_LOCKED);
        query.addColumn(COLUMN_CMD_ID_OFFS);
        query.setPrimary(COLUMN_MOD_NAME);
        query.addUnique(COLUMN_MOD_NAME);
        query.addUnique(COLUMN_MOD_MAIN);

        ret = query.exec();
    }

    if (ret)
    {
        query.setType(Query::CREATE_TABLE, TABLE_IPBANS);
        query.addColumn(COLUMN_IPADDR);
        query.addColumn(COLUMN_TIME);
        query.setPrimary(COLUMN_IPADDR);
        query.addUnique(COLUMN_IPADDR);

        ret = query.exec();
    }

    if (ret)
    {
        query.setType(Query::CREATE_TABLE, TABLE_CMD_RANKS);
        query.addColumn(COLUMN_COMMAND);
        query.addColumn(COLUMN_HOST_RANK);
        query.addColumn(COLUMN_MOD_NAME);
        query.setPrimary(COLUMN_COMMAND);
        query.addUnique(COLUMN_COMMAND);

        ret = query.exec();
    }

    if (ret)
    {
        query.setType(Query::CREATE_TABLE, TABLE_CERT_DATA);
        query.addColumn(COLUMN_COMMON_NAME);
        query.addColumn(COLUMN_CERT);
        query.addColumn(COLUMN_PRIV_KEY);
        query.setPrimary(COLUMN_COMMON_NAME);
        query.addUnique(COLUMN_COMMON_NAME);

        ret = query.exec();
    }

    if (ret)
    {
        query.setType(Query::CREATE_TABLE, TABLE_CHANNELS);
        query.addColumn(COLUMN_CHANNEL_ID);
        query.addColumn(COLUMN_CHANNEL_NAME);
        query.setPrimaryAsc(COLUMN_CHANNEL_ID);
        query.addUnique(COLUMN_CHANNEL_NAME);

        ret = query.exec();
    }

    if (ret)
    {
        query.setType(Query::CREATE_TABLE, TABLE_DMESG);
        query.addColumn(COLUMN_TIME);
        query.addColumn(COLUMN_LOGENTRY);

        ret = query.exec();
    }

    if (ret)
    {
        query.setType(Query::CREATE_TABLE, TABLE_GROUPS);
        query.addColumn(COLUMN_GRNAME);
        query.addColumn(COLUMN_HOST_RANK);
        query.setPrimary(COLUMN_GRNAME);
        query.addUnique(COLUMN_GRNAME);

        ret = query.exec();

        if (query.createExecuted())
        {
            query.setType(Query::PUSH, TABLE_GROUPS);
            query.addColumn(COLUMN_GRNAME, ROOT_USER);
            query.addColumn(COLUMN_HOST_RANK, 1);

            ret = query.exec();

            if (ret)
            {
                query.setType(Query::PUSH, TABLE_GROUPS);
                query.addColumn(COLUMN_GRNAME, DEFAULT_UGROUP);
                query.addColumn(COLUMN_HOST_RANK, 2);

                ret = query.exec();
            }
        }
    }

    if (ret)
    {
        query.setType(Query::CREATE_TABLE, TABLE_USERS);
        query.addColumn(COLUMN_USERNAME);
        query.addColumn(COLUMN_USER_ID);
        query.addColumn(COLUMN_GRNAME);
        query.addColumn(COLUMN_EMAIL);
        query.addColumn(COLUMN_HASH);
        query.addColumn(COLUMN_SALT);
        query.addColumn(COLUMN_TIME);
        query.addColumn(COLUMN_NEED_PASS);
        query.addColumn(COLUMN_NEED_NAME);
        query.addColumn(COLUMN_LOCKED);
        query.addColumn(COLUMN_EMAIL_VERIFIED);
        query.addColumn(COLUMN_DISPLAY_NAME);
        query.setPrimary(COLUMN_USERNAME);
        query.addUnique(COLUMN_USERNAME);
        query.addUnique(COLUMN_EMAIL);
        query.addUnique(COLUMN_USER_ID);
        query.addForeign(COLUMN_GRNAME, TABLE_GROUPS, COLUMN_GRNAME, Query::RESTRICT, Query::CASCADE);

        ret = query.exec();

        if (query.createExecuted())
        {
            query.setType(Query::PUSH, TABLE_USERS);
            query.addColumn(COLUMN_USERNAME, ROOT_USER);
            query.addColumn(COLUMN_GRNAME, ROOT_USER);
            query.addColumn(COLUMN_NEED_PASS, true);
            query.addColumn(COLUMN_NEED_NAME, false);
            query.addColumn(COLUMN_LOCKED, false);
            query.addColumn(COLUMN_EMAIL_VERIFIED, false);
            query.addColumn(COLUMN_USER_ID, genUniqueHash());
            query.addRandBlob(COLUMN_SALT, 128);

            ret = query.exec();

            if (ret)
            {
                ret = updatePassword(ROOT_USER, DEFAULT_PASSWRD, TABLE_USERS, true);
            }
        }
    }

    if (ret)
    {
        query.setType(Query::CREATE_TABLE, TABLE_AUTH_LOG);
        query.addColumn(COLUMN_IPADDR);
        query.addColumn(COLUMN_TIME);
        query.addColumn(COLUMN_USERNAME);
        query.addColumn(COLUMN_AUTH_ATTEMPT);
        query.addColumn(COLUMN_RECOVER_ATTEMPT);
        query.addColumn(COLUMN_ACCEPTED);
        query.addColumn(COLUMN_COUNT);
        query.addForeign(COLUMN_USERNAME, TABLE_USERS, COLUMN_USERNAME, Query::CASCADE, Query::CASCADE);

        ret = query.exec();
    }

    if (ret)
    {
        query.setType(Query::CREATE_TABLE, TABLE_PW_RECOVERY);
        query.addColumn(COLUMN_TIME);
        query.addColumn(COLUMN_USERNAME);
        query.addColumn(COLUMN_EMAIL);
        query.addColumn(COLUMN_HASH);
        query.addColumn(COLUMN_SALT);
        query.addColumn(COLUMN_USER_ID);
        query.addUnique(COLUMN_USERNAME);
        query.addUnique(COLUMN_EMAIL);
        query.addForeign(COLUMN_USER_ID, TABLE_USERS, COLUMN_USER_ID, Query::CASCADE, Query::RESTRICT);
        query.addForeign(COLUMN_USERNAME, TABLE_USERS, COLUMN_USERNAME, Query::CASCADE, Query::CASCADE);
        query.addForeign(COLUMN_EMAIL, TABLE_USERS, COLUMN_EMAIL, Query::CASCADE, Query::CASCADE);

        ret = query.exec();
    }

    if (ret)
    {
        query.setType(Query::CREATE_TABLE, TABLE_CH_MEMBERS);
        query.addColumn(COLUMN_CHANNEL_ID);
        query.addColumn(COLUMN_CHANNEL_NAME);
        query.addColumn(COLUMN_USERNAME);
        query.addColumn(COLUMN_PENDING_INVITE);
        query.addColumn(COLUMN_ACCESS_LEVEL);
        query.addForeign(COLUMN_CHANNEL_ID, TABLE_CHANNELS, COLUMN_CHANNEL_ID, Query::CASCADE, Query::CASCADE);
        query.addForeign(COLUMN_CHANNEL_NAME, TABLE_CHANNELS, COLUMN_CHANNEL_NAME, Query::CASCADE, Query::CASCADE);
        query.addForeign(COLUMN_USERNAME, TABLE_USERS, COLUMN_USERNAME, Query::CASCADE, Query::CASCADE);

        ret = query.exec();
    }

    if (ret)
    {
        query.setType(Query::CREATE_TABLE, TABLE_SUB_CHANNELS);
        query.addColumn(COLUMN_CHANNEL_ID);
        query.addColumn(COLUMN_SUB_CH_NAME);
        query.addColumn(COLUMN_SUB_CH_ID);
        query.addColumn(COLUMN_CHANNEL_NAME);
        query.addColumn(COLUMN_LOWEST_LEVEL);
        query.addColumn(COLUMN_ACTIVE_UPDATE);
        query.addForeign(COLUMN_CHANNEL_ID, TABLE_CHANNELS, COLUMN_CHANNEL_ID, Query::CASCADE, Query::CASCADE);
        query.addForeign(COLUMN_CHANNEL_NAME, TABLE_CHANNELS, COLUMN_CHANNEL_NAME, Query::CASCADE, Query::CASCADE);

        ret = query.exec();
    }

    if (ret)
    {
        query.setType(Query::CREATE_TABLE, TABLE_RDONLY_CAST);
        query.addColumn(COLUMN_CHANNEL_ID);
        query.addColumn(COLUMN_CHANNEL_NAME);
        query.addColumn(COLUMN_SUB_CH_ID);
        query.addColumn(COLUMN_ACCESS_LEVEL);
        query.addForeign(COLUMN_CHANNEL_ID, TABLE_CHANNELS, COLUMN_CHANNEL_ID, Query::CASCADE, Query::CASCADE);
        query.addForeign(COLUMN_CHANNEL_NAME, TABLE_CHANNELS, COLUMN_CHANNEL_NAME, Query::CASCADE, Query::CASCADE);
    }

    if (ret)
    {
        query.setType(Query::CREATE_TABLE, TABLE_SERV_SETTINGS);
        query.addColumn(COLUMN_IPADDR);
        query.addColumn(COLUMN_PORT);
        query.addColumn(COLUMN_BAN_LIMIT);
        query.addColumn(COLUMN_LOCK_LIMIT);
        query.addColumn(COLUMN_MAXSESSIONS);
        query.addColumn(COLUMN_PUB_USERS);
        query.addColumn(COLUMN_ZIPBIN);
        query.addColumn(COLUMN_ZIPCOMPRESS);
        query.addColumn(COLUMN_ZIPEXTRACT);
        query.addColumn(COLUMN_INITGROUP);
        query.addColumn(COLUMN_MAILERBIN);
        query.addColumn(COLUMN_MAIL_SEND);
        query.addColumn(COLUMN_CONFIRM_SUBJECT);
        query.addColumn(COLUMN_TEMP_PW_SUBJECT);
        query.addColumn(COLUMN_CONFIRM_MSG);
        query.addColumn(COLUMN_TEMP_PW_MSG);
        query.addColumn(COLUMN_ENABLE_CONFIRM);
        query.addColumn(COLUMN_ENABLE_PW_RESET);
        query.addColumn(COLUMN_ACTIVE_UPDATE);
        query.addColumn(COLUMN_MAX_SUB_CH);
        query.addForeign(COLUMN_INITGROUP, TABLE_GROUPS, COLUMN_GRNAME, Query::RESTRICT, Query::CASCADE);

        ret = query.exec();

        if (query.createExecuted())
        {
            query.setType(Query::PUSH, TABLE_SERV_SETTINGS);
            query.addColumn(COLUMN_IPADDR, DEFAULT_LISTEN_ADDRESS);
            query.addColumn(COLUMN_PORT, DEFAULT_LISTEN_PORT);
            query.addColumn(COLUMN_BAN_LIMIT, DEFAULT_BAN_LIMIT);
            query.addColumn(COLUMN_LOCK_LIMIT, DEFAULT_LOCK_LIMIT);
            query.addColumn(COLUMN_MAXSESSIONS, DEFAULT_MAXSESSIONS);
            query.addColumn(COLUMN_PUB_USERS, false);
            query.addColumn(COLUMN_ZIPBIN, DEFAULT_ZIPBIN);
            query.addColumn(COLUMN_ZIPCOMPRESS, DEFAULT_ZIPCOMPRESS);
            query.addColumn(COLUMN_ZIPEXTRACT, DEFAULT_ZIPEXTRACT);
            query.addColumn(COLUMN_INITGROUP, DEFAULT_UGROUP);
            query.addColumn(COLUMN_MAILERBIN, DEFAULT_MAILBIN);
            query.addColumn(COLUMN_MAIL_SEND, DEFAULT_MAIL_SEND);
            query.addColumn(COLUMN_CONFIRM_SUBJECT, DEFAULT_CONFIRM_SUBJECT);
            query.addColumn(COLUMN_TEMP_PW_SUBJECT, DEFAULT_TEMP_PW_SUBJECT);
            query.addColumn(COLUMN_CONFIRM_MSG, TXT_ConfirmCodeTemplate);
            query.addColumn(COLUMN_TEMP_PW_MSG, TXT_TempPwTemplate);
            query.addColumn(COLUMN_ENABLE_CONFIRM, true);
            query.addColumn(COLUMN_ENABLE_PW_RESET, true);
            query.addColumn(COLUMN_ACTIVE_UPDATE, true);
            query.addColumn(COLUMN_MAX_SUB_CH, DEFAULT_MAX_SUBS);

            ret = query.exec();
        }
        else
        {
            query.setType(Query::PULL, TABLE_SERV_SETTINGS);
            query.addColumn(COLUMN_IPADDR);
            query.addColumn(COLUMN_PORT);
            query.addColumn(COLUMN_BAN_LIMIT);
            query.addColumn(COLUMN_LOCK_LIMIT);
            query.addColumn(COLUMN_MAXSESSIONS);
            query.addColumn(COLUMN_PUB_USERS);
            query.addColumn(COLUMN_ZIPBIN);
            query.addColumn(COLUMN_ZIPCOMPRESS);
            query.addColumn(COLUMN_ZIPEXTRACT);
            query.addColumn(COLUMN_INITGROUP);
            query.addColumn(COLUMN_MAILERBIN);
            query.addColumn(COLUMN_MAIL_SEND);
            query.addColumn(COLUMN_CONFIRM_SUBJECT);
            query.addColumn(COLUMN_TEMP_PW_SUBJECT);
            query.addColumn(COLUMN_CONFIRM_MSG);
            query.addColumn(COLUMN_TEMP_PW_MSG);
            query.addColumn(COLUMN_ENABLE_CONFIRM);
            query.addColumn(COLUMN_ENABLE_PW_RESET);
            query.addColumn(COLUMN_ACTIVE_UPDATE);
            query.addColumn(COLUMN_MAX_SUB_CH);

            ret = query.exec();

            if (ret)
            {
                defaults.setType(Query::UPDATE, TABLE_SERV_SETTINGS);

                if (query.getData(COLUMN_IPADDR).isNull())          defaults.addColumn(COLUMN_IPADDR, DEFAULT_LISTEN_ADDRESS);
                if (query.getData(COLUMN_PORT).isNull())            defaults.addColumn(COLUMN_PORT, DEFAULT_LISTEN_PORT);
                if (query.getData(COLUMN_BAN_LIMIT).isNull())       defaults.addColumn(COLUMN_BAN_LIMIT, DEFAULT_BAN_LIMIT);
                if (query.getData(COLUMN_LOCK_LIMIT).isNull())      defaults.addColumn(COLUMN_LOCK_LIMIT, DEFAULT_LOCK_LIMIT);
                if (query.getData(COLUMN_MAXSESSIONS).isNull())     defaults.addColumn(COLUMN_MAXSESSIONS, DEFAULT_MAXSESSIONS);
                if (query.getData(COLUMN_PUB_USERS).isNull())       defaults.addColumn(COLUMN_PUB_USERS, false);
                if (query.getData(COLUMN_ZIPBIN).isNull())          defaults.addColumn(COLUMN_ZIPBIN, DEFAULT_ZIPBIN);
                if (query.getData(COLUMN_ZIPCOMPRESS).isNull())     defaults.addColumn(COLUMN_ZIPCOMPRESS, DEFAULT_ZIPCOMPRESS);
                if (query.getData(COLUMN_ZIPEXTRACT).isNull())      defaults.addColumn(COLUMN_ZIPEXTRACT, DEFAULT_ZIPEXTRACT);
                if (query.getData(COLUMN_INITGROUP).isNull())       defaults.addColumn(COLUMN_INITGROUP, DEFAULT_UGROUP);
                if (query.getData(COLUMN_MAILERBIN).isNull())       defaults.addColumn(COLUMN_MAILERBIN, DEFAULT_MAILBIN);
                if (query.getData(COLUMN_MAIL_SEND).isNull())       defaults.addColumn(COLUMN_MAIL_SEND, DEFAULT_MAIL_SEND);
                if (query.getData(COLUMN_CONFIRM_SUBJECT).isNull()) defaults.addColumn(COLUMN_CONFIRM_SUBJECT, DEFAULT_CONFIRM_SUBJECT);
                if (query.getData(COLUMN_TEMP_PW_SUBJECT).isNull()) defaults.addColumn(COLUMN_TEMP_PW_SUBJECT, DEFAULT_TEMP_PW_SUBJECT);
                if (query.getData(COLUMN_CONFIRM_MSG).isNull())     defaults.addColumn(COLUMN_CONFIRM_MSG, TXT_ConfirmCodeTemplate);
                if (query.getData(COLUMN_TEMP_PW_MSG).isNull())     defaults.addColumn(COLUMN_TEMP_PW_MSG, TXT_TempPwTemplate);
                if (query.getData(COLUMN_ENABLE_CONFIRM).isNull())  defaults.addColumn(COLUMN_ENABLE_CONFIRM, true);
                if (query.getData(COLUMN_ENABLE_PW_RESET).isNull()) defaults.addColumn(COLUMN_ENABLE_PW_RESET, true);
                if (query.getData(COLUMN_ACTIVE_UPDATE).isNull())   defaults.addColumn(COLUMN_ACTIVE_UPDATE, true);
                if (query.getData(COLUMN_MAX_SUB_CH).isNull())      defaults.addColumn(COLUMN_MAX_SUB_CH, DEFAULT_MAX_SUBS);

                if (defaults.columns())
                {
                    ret = defaults.exec();
                }
            }
        }
    }

    if (query.inErrorstate())
    {
        errMsg->append("main setup: \n");
        errMsg->append(query.errDetail());
    }

    if (defaults.inErrorstate())
    {
        errMsg->append("setup of default parameters: \n");
        errMsg->append(defaults.errDetail());
    }

    return ret;
}
