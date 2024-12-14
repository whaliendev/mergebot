package space.whalien.conflictmanager.pojo.vo;

public class BlockResolutionChoiceVO {
    private String choice;
    private String choiceCode;

    private String saCode;

    private String mlCode;
    private String dlCode;

    public BlockResolutionChoiceVO(String choice, String choiceCode, String saCode, String mlCode, String dlCode) {
        this.choice = choice;
        this.choiceCode = choiceCode;
        this.saCode = saCode;
        this.mlCode = mlCode;
        this.dlCode = dlCode;
    }

    public String getChoice() {
        return choice;
    }

    public void setChoice(String choice) {
        this.choice = choice;
    }

    public String getChoiceCode() {
        return choiceCode;
    }

    public void setChoiceCode(String choiceCode) {
        this.choiceCode = choiceCode;
    }

    public String getSaCode() {
        return saCode;
    }

    public void setSaCode(String saCode) {
        this.saCode = saCode;
    }

    public String getMlCode() {
        return mlCode;
    }

    public void setMlCode(String mlCode) {
        this.mlCode = mlCode;
    }

    public String getDlCode() {
        return dlCode;
    }

    public void setDlCode(String dlCode) {
        this.dlCode = dlCode;
    }
}
