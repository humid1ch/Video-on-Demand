#include "dataManage.hpp"
#include <json/value.h>
#include <string>

void insertTest(aod::tableVideo& tbv) {
	Json::Value video1;
	video1["v_title"] = "桃花侠大战菊花怪";
	video1["v_info"] = "爱情公寓大电影: 桃花侠大战菊花怪!";
	video1["v_path"] = "./video/peachBlossom_VS_chrysanthemum.mp4";
	video1["v_cover"] = "./cover/peachBlossom_VS_chrysanthemum.webp";
	Json::Value video2;
	video2["v_title"] = "刻晴大战史莱姆";
	video2["v_info"] = "原神大电影!******, 激情四射!";
	video2["v_path"] = "./video/KeQing_VS_Slime.mp4";
	video2["v_cover"] = "./cover/KeQing_VS_Slime.webp";

	tbv.insertVideo(video1);
	tbv.insertVideo(video2);
}

void updateTest(aod::tableVideo& tbv) {
	Json::Value video;
	video["v_title"] = "刻晴大战史莱姆";
	video["v_info"] = "原神大电影! 结拜狱卒, 激情四射!";
	tbv.updateVideo(4, video);
}

void deleteTest(aod::tableVideo& tbv) {
	tbv.deleteVideo(4);
	tbv.deleteVideo(6);
	tbv.deleteVideo(8);
}

void selectTest(aod::tableVideo& tbv) {
	Json::Value selectOneRes;
	Json::Value selectAllRes;
	Json::Value selectLikeRes;
	tbv.selectOneVideo(7, &selectOneRes);
	tbv.selectAllVideo(&selectAllRes);
	tbv.selectLikeVideo("大战", &selectLikeRes);

	std::string serOne;
	std::string serAll;
	std::string serLike;
	aod::jsonUtil::serialize(selectOneRes, &serOne);
	aod::jsonUtil::serialize(selectLikeRes, &serLike);
	aod::jsonUtil::serialize(selectAllRes, &serAll);

	std::cout << "selectOne:: \n"
			  << serOne << std::endl;
	std::cout << "selectAll:: \n"
			  << serAll << std::endl;
	std::cout << "selectLike:: \n"
			  << serLike << std::endl;
}

int main() {
	aod::tableVideo tbV;

	// insertTest(tbV);
	// updateTest(tbV);
	// deleteTest(tbV);
	selectTest(tbV);

	return 0;
}
