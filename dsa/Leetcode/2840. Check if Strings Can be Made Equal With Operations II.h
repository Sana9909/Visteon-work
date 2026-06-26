class Solution {
public:
    bool checkStrings(string s1, string s2) {
        int n=s1.size();
        int m=s2.size();

        if(n!=m){
            return false;
        }

        vector<int> even(26,0);
        vector<int> odd(26,0);

        for(int i=0;i<n;i++){
            if(i%2){
                odd[(int)s1[i]-97]++;
                odd[(int)s2[i]-97]--;
            }
            else{
                even[(int)s1[i]-97]++;
                even[(int)s2[i]-97]--;
            }
        }
        for(int i=0;i<26;i++){
            if(odd[i]!=0 || even[i]!=0){
                return false;
            }
        }
        return true;
    }
};