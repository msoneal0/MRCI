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
    auto ret = true;

    errMsg->clear();

    Query query(QThread::currentThread());

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
        query.addColumn(COLUMN_SESSION_ID);
        query.addColumn(COLUMN_APP_NAME);
        query.addColumn(COLUMN_LOGENTRY);

        ret = query.exec();
    }

    if (ret)
    {
        query.setType(Query::CREATE_TABLE, TABLE_MODULES);
        query.addColumn(COLUMN_MOD_MAIN);
        query.addUnique(COLUMN_MOD_MAIN);

        ret = query.exec();
    }

    if (ret)
    {
        query.setType(Query::CREATE_TABLE, TABLE_CMD_RANKS);
        query.addColumn(COLUMN_COMMAND);
        query.addColumn(COLUMN_HOST_RANK);
        query.addColumn(COLUMN_MOD_MAIN);

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
        query.setType(Query::CREATE_TABLE, TABLE_USERS);
        query.addColumn(COLUMN_USERNAME);
        query.addColumn(COLUMN_USER_ID);
        query.addColumn(COLUMN_EMAIL);
        query.addColumn(COLUMN_HASH);
        query.addColumn(COLUMN_SALT);
        query.addColumn(COLUMN_TIME);
        query.addColumn(COLUMN_NEED_PASS);
        query.addColumn(COLUMN_NEED_NAME);
        query.addColumn(COLUMN_LOCKED);
        query.addColumn(COLUMN_EMAIL_VERIFIED);
        query.addColumn(COLUMN_DISPLAY_NAME);
        query.addColumn(COLUMN_HOST_RANK);
        query.setPrimary(COLUMN_USER_ID);
        query.addUnique(COLUMN_USERNAME);
        query.addUnique(COLUMN_EMAIL);
        query.addUnique(COLUMN_USER_ID);

        ret = query.exec();
    }

    if (ret)
    {
        query.setType(Query::CREATE_TABLE, TABLE_AUTH_LOG);
        query.addColumn(COLUMN_IPADDR);
        query.addColumn(COLUMN_TIME);
        query.addColumn(COLUMN_USER_ID);
        query.addColumn(COLUMN_AUTH_ATTEMPT);
        query.addColumn(COLUMN_RECOVER_ATTEMPT);
        query.addColumn(COLUMN_ACCEPTED);
        query.addColumn(COLUMN_COUNT);

        ret = query.exec();
    }

    if (ret)
    {
        query.setType(Query::CREATE_TABLE, TABLE_PW_RECOVERY);
        query.addColumn(COLUMN_TIME);
        query.addColumn(COLUMN_HASH);
        query.addColumn(COLUMN_SALT);
        query.addColumn(COLUMN_USER_ID);
        query.addForeign(COLUMN_USER_ID, TABLE_USERS, COLUMN_USER_ID, Query::CASCADE, Query::CASCADE);

        ret = query.exec();
    }

    if (ret)
    {
        query.setType(Query::CREATE_TABLE, TABLE_CH_MEMBERS);
        query.addColumn(COLUMN_CHANNEL_ID);
        query.addColumn(COLUMN_USER_ID);
        query.addColumn(COLUMN_PENDING_INVITE);
        query.addColumn(COLUMN_ACCESS_LEVEL);
        query.addForeign(COLUMN_CHANNEL_ID, TABLE_CHANNELS, COLUMN_CHANNEL_ID, Query::CASCADE, Query::CASCADE);
        query.addForeign(COLUMN_USER_ID, TABLE_USERS, COLUMN_USER_ID, Query::CASCADE, Query::CASCADE);

        ret = query.exec();
    }

    if (ret)
    {
        query.setType(Query::CREATE_TABLE, TABLE_SUB_CHANNELS);
        query.addColumn(COLUMN_CHANNEL_ID);
        query.addColumn(COLUMN_SUB_CH_NAME);
        query.addColumn(COLUMN_SUB_CH_ID);
        query.addColumn(COLUMN_LOWEST_LEVEL);
        query.addColumn(COLUMN_ACTIVE_UPDATE);
        query.addForeign(COLUMN_CHANNEL_ID, TABLE_CHANNELS, COLUMN_CHANNEL_ID, Query::CASCADE, Query::CASCADE);

        ret = query.exec();
    }

    if (ret)
    {
        query.setType(Query::CREATE_TABLE, TABLE_RDONLY_CAST);
        query.addColumn(COLUMN_CHANNEL_ID);
        query.addColumn(COLUMN_SUB_CH_ID);
        query.addColumn(COLUMN_ACCESS_LEVEL);
        query.addForeign(COLUMN_CHANNEL_ID, TABLE_CHANNELS, COLUMN_CHANNEL_ID, Query::CASCADE, Query::CASCADE);

        ret = query.exec();
    }

    if (query.inErrorstate())
    {
        errMsg->append("main setup: \n");
        errMsg->append(query.errDetail());
    }

    return ret;
}
