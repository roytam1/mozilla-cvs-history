typedef struct s_component {
	CString archive;
	CString compname;
	CString name;
	CString desc;
	BOOL	selected;
	BOOL	invisible;
	BOOL	launchapp;
	BOOL	additional;
	} COMPONENT;

extern "C" __declspec(dllexport) 
int BuildComponentList(COMPONENT *comps, int *compNum, CString iniSrcPath,int invisibleCount);
