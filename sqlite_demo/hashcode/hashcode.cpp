/*

** Comparision function for two Keyword records

*/

static int keywordCompare1(const void* a, const void* b) {

	const Keyword* pA = (Keyword*)a;

	const Keyword* pB = (Keyword*)b;

	int n = pA->len - pB->len;

	if (n == 0) {

		n = strcmp(pA->zName, pB->zName);

	}

	assert(n != 0);

	return n;

}

static int keywordCompare2(const void* a, const void* b) {

	const Keyword* pA = (Keyword*)a;

	const Keyword* pB = (Keyword*)b;

	int n = pB->longestSuffix - pA->longestSuffix;

	if (n == 0) {

		n = strcmp(pA->zName, pB->zName);

	}

	assert(n != 0);

	return n;

}

static int keywordCompare3(const void* a, const void* b) {

	const Keyword* pA = (Keyword*)a;

	const Keyword* pB = (Keyword*)b;

	int n = pA->offset - pB->offset;

	if (n == 0) n = pB->id - pA->id;

	assert(n != 0);

	return n;

}

