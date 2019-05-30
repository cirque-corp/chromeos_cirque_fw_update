# Copyright (c) 2019 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

cirque_touch_fw_update: clean
	g++ -std=c++0x CirqueBootloaderCollection.cpp CirqueDevData.cpp CirqueHexFileRecord.cpp CirqueHexFileParser.cpp CirqueTouchFwUpdater.cpp -o cirque_touch_fw_update -static

clean:
	-rm cirque_touch_fw_update
