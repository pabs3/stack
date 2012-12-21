#define DEBUG_TYPE "inline-only"
#include <llvm/DataLayout.h>
#include <llvm/Module.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/Analysis/InlineCost.h>
#include <llvm/Transforms/IPO/InlinerPass.h>

using namespace llvm;

namespace {

struct InlineOnly : Inliner {
	static char ID;
	InlineOnly() : Inliner(ID) {}

	virtual InlineCost getInlineCost(CallSite CS) {
		return CA.getInlineCost(CS, getInlineThreshold(CS));
	}

	virtual bool doInitialization(CallGraph &CG);
	virtual bool doFinalization(CallGraph &CG);

private:
	InlineCostAnalyzer CA;
	typedef DenseMap<Function *, GlobalValue::LinkageTypes> LinkageMapTy;
	LinkageMapTy LinkageMap;
};

} // anonymous namespace

bool InlineOnly::doInitialization(CallGraph &CG) {
	CA.setDataLayout(getAnalysisIfAvailable<DataLayout>());
	Module &M = CG.getModule();
	LinkageMap.clear();
	// Temporarily change local functions to linkonce_odr.
	// Inliner handles linkonce_odr in the same way as static,
	// except that it doesn't remove linkonce_odr functions.
	for (Module::iterator i = M.begin(), e = M.end(); i != e; ++i) {
		Function *F = i;
		if (F->hasLocalLinkage()) {
			LinkageMap[F] = F->getLinkage();
			F->setLinkage(GlobalValue::LinkOnceODRLinkage);
		}
	}
	return !LinkageMap.empty();
}

bool InlineOnly::doFinalization(CallGraph &) {
	// Restore original linkage.
	for (LinkageMapTy::iterator i = LinkageMap.begin(), e = LinkageMap.end(); i != e; ++i)
		i->first->setLinkage(i->second);
	return !LinkageMap.empty();
}

char InlineOnly::ID;

static RegisterPass<InlineOnly>
X("inline-only", "Inline functions but never remove inlined ones");
